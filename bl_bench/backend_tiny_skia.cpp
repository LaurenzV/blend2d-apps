#include "backend_tiny_skia.h"

namespace blbench {
    struct TinySkiaModule : public Backend {
        ts_pixmap* pixmap {};
        ts_stroke stroke;

        TinySkiaModule();
        ~TinySkiaModule() override;

        bool supportsCompOp(BLCompOp compOp) const override;
        bool supportsStyle(StyleKind style) const override;

        void beforeRun() override;
        void flush() override;
        void afterRun() override;

        void renderRectA(RenderOp op) override;
        void renderRectF(RenderOp op) override;
        void renderRectRotated(RenderOp op) override;
        void renderRoundF(RenderOp op) override;
        void renderRoundRotated(RenderOp op) override;
        void renderPolygon(RenderOp op, uint32_t complexity) override;
        void renderShape(RenderOp op, ShapeData shape) override;

        inline ts_paint convert_style(const ts_rect& rect, StyleKind style, ts_transform t);
        ts_color gen_color();
        ts_rect convert_rect(BLRect rect);
        ts_rect convert_rect_i(BLRectI rect);
        ts_point convert_point(BLPoint rect);

        ts_blend_mode toTinySkiaOperator(uint32_t compOp);
    };

    TinySkiaModule::TinySkiaModule() {
        strcpy(_name, "tiny-skia");
    }

    TinySkiaModule::~TinySkiaModule() {}

    bool TinySkiaModule::supportsCompOp(BLCompOp compOp) const {
        return compOp == BL_COMP_OP_SRC_OVER || compOp == BL_COMP_OP_SRC_COPY;
    }

    bool TinySkiaModule::supportsStyle(StyleKind style) const {
        return style <= StyleKind::kLinearReflect;
    }

    void TinySkiaModule::beforeRun() {
        int w = int(_params.screenW);
        int h = int(_params.screenH);

        pixmap = ts_pixmap_create(w, h);
        stroke = ts_stroke {(float) _params.strokeWidth };
    }

    void TinySkiaModule::afterRun() {
        int w = int(_params.screenW);
        int h = int(_params.screenH);

        BLImageData dstData;
        _surface.create(int(w), int(h), BL_FORMAT_PRGB32);
        _surface.makeMutable(&dstData);

        auto data = ts_data(pixmap);
        auto bytes = ts_argb_data(data);

        memcpy(
                static_cast<uint8_t*>(dstData.pixelData),
                bytes,
                w * h * 4);
        ts_argb_destroy(data);
        ts_pixmap_destroy(pixmap);
    }

    void TinySkiaModule::flush() {

    }

    void TinySkiaModule::renderRectA(RenderOp op) {
        ts_transform t = ts_transform_identity();
        BLSizeI bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        int wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            ts_rect rect = convert_rect_i(_rndCoord.nextRectI(bounds, wh, wh));
            ts_paint paint = convert_style(rect, style, t);

            if (op == RenderOp::kStroke) {
                ts_pixmap_stroke_rect(pixmap, rect, t, paint, stroke, toTinySkiaOperator(_params.compOp));
            }   else {
                ts_pixmap_fill_rect(pixmap, rect, t, paint, toTinySkiaOperator(_params.compOp));
            }

            ts_paint_destroy(paint);
        }
    }

    void TinySkiaModule::renderRectF(RenderOp op) {
        ts_transform t = ts_transform_identity();
        BLSizeI bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        int wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            ts_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));
            ts_paint paint = convert_style(rect, style, t);

            if (op == RenderOp::kStroke) {
                ts_pixmap_stroke_rect(pixmap, rect, t, paint, stroke, toTinySkiaOperator(_params.compOp));
            }   else {
                ts_pixmap_fill_rect(pixmap, rect, t, paint, toTinySkiaOperator(_params.compOp));
            }

            ts_paint_destroy(paint);
        }
    }

    void TinySkiaModule::renderRectRotated(RenderOp op) {
        BLSize bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        ts_transform id = ts_transform_identity();

        double cx = double(_params.screenW) * 0.5;
        double cy = double(_params.screenH) * 0.5;
        double wh = _params.shapeSize;
        double angle = 0.0;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
            ts_transform t = ts_transform_rotate_at(angle * 180.0 / 3.141592653, cx, cy);
            ts_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));
            ts_paint paint = convert_style(rect, style, id);

            if (op == RenderOp::kStroke) {
                ts_pixmap_stroke_rect(pixmap, rect, t, paint, stroke, toTinySkiaOperator(_params.compOp));
            }   else {
                ts_pixmap_fill_rect(pixmap, rect, t, paint, toTinySkiaOperator(_params.compOp));
            }

            ts_paint_destroy(paint);
        }
    }

    void TinySkiaModule::renderRoundF(RenderOp op) {
        BLSize bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        ts_transform t = ts_transform_identity();
        double wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            double radius = _rndExtra.nextDouble(4.0, 40.0);
            ts_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));
            ts_paint paint = convert_style(rect, style, t);

            ts_path *p = ts_rounded_rect(rect, (float) radius, (float) radius);

            if (op == RenderOp::kStroke) {
                ts_pixmap_stroke_path(pixmap, p, t, paint, stroke, toTinySkiaOperator(_params.compOp));
            }   else {
                ts_pixmap_fill_path(pixmap, p, t, paint, ts_fill_rule::Winding, toTinySkiaOperator(_params.compOp));
            }

            ts_paint_destroy(paint);
            ts_path_destroy(p);
        }
    }

    void TinySkiaModule::renderRoundRotated(RenderOp op) {
        BLSize bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        ts_transform t = ts_transform_identity();

        double cx = double(_params.screenW) * 0.5;
        double cy = double(_params.screenH) * 0.5;
        double wh = _params.shapeSize;
        double angle = 0.0;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
            ts_transform t = ts_transform_rotate_at(angle * 180.0 / 3.141592653, cx, cy);
            double radius = _rndExtra.nextDouble(4.0, 40.0);
            ts_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));
            ts_paint paint = convert_style(rect, style, t);

            ts_path *p = ts_rounded_rect(rect, (float) radius, (float) radius);

            if (op == RenderOp::kStroke) {
                ts_pixmap_stroke_path(pixmap, p, t, paint, stroke, toTinySkiaOperator(_params.compOp));
            }   else {
                ts_pixmap_fill_path(pixmap, p, t, paint, ts_fill_rule::Winding, toTinySkiaOperator(_params.compOp));
            }

            ts_paint_destroy(paint);
            ts_path_destroy(p);
        }
    }

    void TinySkiaModule::renderPolygon(RenderOp op, uint32_t complexity) {
        BLSizeI bounds(_params.screenW - _params.shapeSize,
                       _params.screenH - _params.shapeSize);
        ts_transform t = ts_transform_identity();
        float wh = (float) _params.shapeSize;
        StyleKind style = _params.style;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            ts_point base = convert_point(_rndCoord.nextPoint(bounds));
            ts_rect base_rect = {base.x, base.y, base.x + wh, base.y + wh};
            ts_paint paint = convert_style(base_rect, style, t);

            double x = _rndCoord.nextDouble(base.x, base.x + wh);
            double y = _rndCoord.nextDouble(base.y, base.y + wh);

            ts_path_builder *builder = ts_path_builder_create();
            ts_move_to(builder, x, y);
            for (uint32_t p = 1; p < complexity; p++) {
                x = _rndCoord.nextDouble(base.x, base.x + wh);
                y = _rndCoord.nextDouble(base.y, base.y + wh);
                ts_line_to(builder, x, y);
            }

            ts_close(builder);

            ts_path *path = ts_path_builder_finish(builder);
            ts_fill_rule fr = (op == RenderOp::kFillEvenOdd ? ts_fill_rule::EvenOdd : ts_fill_rule::Winding);

            if (op == RenderOp::kStroke) {
                ts_pixmap_stroke_path(pixmap, path, t, paint, stroke, toTinySkiaOperator(_params.compOp));
            }   else {
                ts_pixmap_fill_path(pixmap, path, t, paint, fr, toTinySkiaOperator(_params.compOp));
            }

            ts_paint_destroy(paint);
            ts_path_destroy(path);
        }
    }

    void TinySkiaModule::renderShape(RenderOp op, ShapeData shape) {
        BLSizeI bounds(_params.screenW - _params.shapeSize, _params.screenH - _params.shapeSize);
        StyleKind style = _params.style;
        double wh = double(_params.shapeSize);

        ts_path_builder *builder = ts_path_builder_create();
        ShapeIterator it(shape);

        while (it.hasCommand()) {
            if (it.isMoveTo()) {
                ts_move_to(builder, it.x(0) * wh, it.y(0) * wh);
            }
            else if (it.isLineTo()) {
                ts_line_to(builder, it.x(0) * wh, it.y(0) * wh);
            }
            else if (it.isQuadTo()) {
                ts_quad_to(builder, it.x(0) * wh, it.y(0) * wh,
                           it.x(1) * wh, it.y(1) * wh);
            }
            else if (it.isCubicTo()) {
                ts_cubic_to(builder, it.x(0) * wh, it.y(0) * wh,
                            it.x(1) * wh, it.y(1) * wh,
                            it.x(2) * wh, it.y(2) * wh);
            }
            else {
                ts_close(builder);
            }

            it.next();
        }

        ts_path *path = ts_path_builder_finish(builder);
        ts_fill_rule fr = (op == RenderOp::kFillEvenOdd ? ts_fill_rule::EvenOdd : ts_fill_rule::Winding);

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            BLPoint base(_rndCoord.nextPoint(bounds));
            ts_rect base_rect = { (float) base.x, (float) base.y, (float) (base.x + wh), (float) (base.y + wh)};
            ts_transform t = ts_transform_translate(base.x, base.y);
            ts_transform inv_t = ts_transform_translate(-base.x, -base.y);
            ts_paint paint = convert_style(base_rect, style, inv_t);


            if (op == RenderOp::kStroke) {
                ts_pixmap_stroke_path(pixmap, path, t, paint, stroke, toTinySkiaOperator(_params.compOp));
            }   else {
                ts_pixmap_fill_path(pixmap, path, t, paint, fr, toTinySkiaOperator(_params.compOp));
            }

            ts_paint_destroy(paint);
        }

        ts_path_destroy(path);
    }

    ts_color TinySkiaModule::gen_color() {
        auto bl_color = _rndColor.nextRgba32();
        ts_color color = {(uint8_t) bl_color.r(), (uint8_t) bl_color.g(), (uint8_t) bl_color.b(), (uint8_t) bl_color.a()};

        return color;
    }

    inline ts_paint TinySkiaModule::convert_style(const ts_rect& rect, StyleKind style, ts_transform t) {
        ts_spread_mode mode;

        float w = rect.x1 - rect.x0;
        float h = rect.y1 - rect.y0;

        switch(style) {
            case StyleKind::kLinearReflect:
            case StyleKind::kRadialReflect:
                mode = ts_spread_mode::Reflect;
                break;
            case StyleKind::kLinearRepeat:
            case StyleKind::kRadialRepeat:
                mode = ts_spread_mode::Repeat;
                break;
            default:
                mode = ts_spread_mode::Pad;
        }

        switch (style) {
            case StyleKind::kSolid: {
                ts_color color = gen_color();

                ts_paint paint;
                paint.tag = ts_paint::Tag::Color;
                paint.color = ts_paint::Color_Body{ color };
                return paint;
            }
            case StyleKind::kLinearPad:
            case StyleKind::kLinearRepeat:
            case StyleKind::kLinearReflect: {
                ts_color c0 = gen_color();
                ts_color c1 = gen_color();
                ts_color c2 = gen_color();

                float x0 = rect.x0 + w * 0.2;
                float y0 = rect.y0 + h * 0.2;
                float x1 = rect.x0 + w * 0.8;
                float y1 = rect.y0 + h * 0.8;

                ts_linear_gradient *grad = ts_linear_gradient_create(x0, y0, x1, y1, mode, t);

                ts_linear_gradient_push_stop(grad, {0.0, c0});
                ts_linear_gradient_push_stop(grad, {0.5, c1});
                ts_linear_gradient_push_stop(grad, {1.0, c2});

                ts_paint paint;
                paint.tag = ts_paint::Tag::LinearGradient;
                paint.linear_gradient = ts_paint::LinearGradient_Body{ grad };
                return paint;
            }
            default: {
                ts_color color = gen_color();

                ts_paint paint;
                paint.tag = ts_paint::Tag::Color;
                paint.color = ts_paint::Color_Body{ color };
                return paint;
            }
        }
    }

    ts_rect TinySkiaModule::convert_rect(BLRect bl_rect) {
        return {(float) bl_rect.x, (float) bl_rect.y, (float) (bl_rect.x + bl_rect.w), (float) (bl_rect.y + bl_rect.h)};
    }

    ts_point TinySkiaModule::convert_point(BLPoint point) {
        return { (float) point.x, (float) point.y };
    }

    ts_rect TinySkiaModule::convert_rect_i(BLRectI bl_rect) {
        return {(float) bl_rect.x, (float) bl_rect.y, (float) (bl_rect.x + bl_rect.w), (float) (bl_rect.y + bl_rect.h)};
    }

    ts_blend_mode TinySkiaModule::toTinySkiaOperator(uint32_t compOp) {
        switch (compOp) {
            case BL_COMP_OP_SRC_OVER   : return ts_blend_mode::SourceOver;
            case BL_COMP_OP_SRC_COPY   : return ts_blend_mode::SourceCopy;
            default:
                return ts_blend_mode::SourceOver;
        }
    }

    Backend* createTinySkiaBackend() {
        return new TinySkiaModule();
    }
}

