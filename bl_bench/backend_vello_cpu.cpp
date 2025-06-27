#ifdef BLEND2D_APPS_ENABLE_VELLO_CPU

#include "backend_vello_cpu.h"

namespace blbench {
    struct VelloCpuModule : public Backend {
        vc_context* context {};
        vc_pixmap* pixmap {};
        vc_stroke stroke;

        VelloCpuModule();
        ~VelloCpuModule() override;

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

        inline vc_paint convert_style(const vc_rect& rect, StyleKind style, vc_transform t);
        vc_color gen_color();
        vc_rect convert_rect(BLRect rect);
        vc_rect convert_rect_i(BLRectI rect);
        vc_point convert_point(BLPoint rect);
    };

    VelloCpuModule::VelloCpuModule() {
        strcpy(_name, "vello-cpu");
    }

    VelloCpuModule::~VelloCpuModule() {}

    bool VelloCpuModule::supportsCompOp(BLCompOp compOp) const {
        return compOp == BL_COMP_OP_SRC_OVER;
    }

    bool VelloCpuModule::supportsStyle(StyleKind style) const {
        return style <= StyleKind::kSolid;
    }

    void VelloCpuModule::beforeRun() {
        int w = int(_params.screenW);
        int h = int(_params.screenH);

        pixmap = vc_pixmap_create(w, h);
        context = vc_context_create(w, h);
        stroke = vc_stroke { _params.strokeWidth };
    }

    void VelloCpuModule::afterRun() {
        int w = int(_params.screenW);
        int h = int(_params.screenH);

        BLImageData dstData;
        _surface.create(int(w), int(h), BL_FORMAT_PRGB32);
        _surface.makeMutable(&dstData);

        auto data = vc_data(pixmap);
        auto bytes = vc_argb_data(data);

        memcpy(
                static_cast<uint8_t*>(dstData.pixelData),
                bytes,
                w * h * 4);
        vc_argb_destroy(data);
        vc_pixmap_destroy(pixmap);
        vc_context_destroy(context);
    }

    void VelloCpuModule::flush() {

    }

    void VelloCpuModule::renderRectA(RenderOp op) {
        vc_transform t = vc_transform_identity();
        BLSizeI bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        int wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            vc_rect rect = convert_rect_i(_rndCoord.nextRectI(bounds, wh, wh));
            vc_paint paint = convert_style(rect, style, t);
            vc_set_paint(context, paint);

            if (op == RenderOp::kStroke) {
                vc_set_stroke(context, stroke);
                vc_stroke_rect(context, rect);
            }   else {
                vc_fill_rect(context, rect);
            }
        }

        vc_render_to_pixmap(pixmap, context);
    }

    void VelloCpuModule::renderRectF(RenderOp op) {
        vc_transform t = vc_transform_identity();
        BLSizeI bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        int wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            vc_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));
            vc_paint paint = convert_style(rect, style, t);
            vc_set_paint(context, paint);

            if (op == RenderOp::kStroke) {
                vc_set_stroke(context, stroke);
                vc_stroke_rect(context, rect);
            }   else {
                vc_fill_rect(context, rect);
            }
        }

        vc_render_to_pixmap(pixmap, context);
    }

    void VelloCpuModule::renderRectRotated(RenderOp op) {
        BLSize bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        vc_transform id = vc_transform_identity();

        double cx = double(_params.screenW) * 0.5;
        double cy = double(_params.screenH) * 0.5;
        double wh = _params.shapeSize;
        double angle = 0.0;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
            vc_transform t = vc_transform_rotate_at(angle, cx, cy);
            vc_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));
            vc_paint paint = convert_style(rect, style, id);

            vc_set_paint(context, paint);
            vc_set_transform(context, t);

            if (op == RenderOp::kStroke) {
                vc_set_stroke(context, stroke);
                vc_stroke_rect(context, rect);
            }   else {
                vc_fill_rect(context, rect);
            }

        }

        vc_render_to_pixmap(pixmap, context);
    }

    void VelloCpuModule::renderRoundF(RenderOp op) {
        BLSize bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        vc_transform t = vc_transform_identity();
        double wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            double radius = _rndExtra.nextDouble(4.0, 40.0);
            vc_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));
            vc_paint paint = convert_style(rect, style, t);

            vc_path *p = vc_rounded_rect(rect, radius);
            vc_set_paint(context, paint);

            if (op == RenderOp::kStroke) {
                vc_set_stroke(context, stroke);
                vc_stroke_path(context, p);
            }   else {
                vc_fill_path(context, p);
            }

            vc_path_destroy(p);
        }

        vc_render_to_pixmap(pixmap, context);
    }

    void VelloCpuModule::renderRoundRotated(RenderOp op) {
        BLSize bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;

        double cx = double(_params.screenW) * 0.5;
        double cy = double(_params.screenH) * 0.5;
        double wh = _params.shapeSize;
        double angle = 0.0;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
            vc_transform t = vc_transform_rotate_at(angle, cx, cy);
            double radius = _rndExtra.nextDouble(4.0, 40.0);
            vc_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));
            vc_paint paint = convert_style(rect, style, t);

            vc_path *p = vc_rounded_rect(rect, radius);

            vc_set_transform(context, t);
            vc_set_paint(context, paint);

            if (op == RenderOp::kStroke) {
                vc_set_stroke(context, stroke);
                vc_stroke_path(context, p);
            }   else {
                vc_fill_path(context, p);
            }

            vc_path_destroy(p);
        }

        vc_render_to_pixmap(pixmap, context);
    }

    void VelloCpuModule::renderPolygon(RenderOp op, uint32_t complexity) {
        BLSizeI bounds(_params.screenW - _params.shapeSize,
                       _params.screenH - _params.shapeSize);
        vc_transform t = vc_transform_identity();
        float wh = (float) _params.shapeSize;
        StyleKind style = _params.style;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            vc_point base = convert_point(_rndCoord.nextPoint(bounds));
            vc_rect base_rect = {base.x, base.y, base.x + wh, base.y + wh};
            vc_paint paint = convert_style(base_rect, style, t);

            double x = _rndCoord.nextDouble(base.x, base.x + wh);
            double y = _rndCoord.nextDouble(base.y, base.y + wh);

            vc_path *path = vc_path_create();
            vc_move_to(path, vc_point {x, y});
            for (uint32_t p = 1; p < complexity; p++) {
                x = _rndCoord.nextDouble(base.x, base.x + wh);
                y = _rndCoord.nextDouble(base.y, base.y + wh);
                vc_line_to(path, vc_point {x, y});
            }

            vc_close(path);

            vc_fill_rule fr = (op == RenderOp::kFillEvenOdd ? vc_fill_rule::EvenOdd : vc_fill_rule::Winding);

            vc_set_paint(context, paint);
            vc_set_fill_rule(context, fr);

            if (op == RenderOp::kStroke) {
                vc_set_stroke(context, stroke);
                vc_stroke_path(context, path);
            }   else {
                vc_fill_path(context, path);
            }

            vc_path_destroy(path);
        }

        vc_render_to_pixmap(pixmap, context);
    }

    void VelloCpuModule::renderShape(RenderOp op, ShapeData shape) {
        BLSizeI bounds(_params.screenW - _params.shapeSize, _params.screenH - _params.shapeSize);
        StyleKind style = _params.style;
        double wh = double(_params.shapeSize);

        vc_path *path = vc_path_create();
        ShapeIterator it(shape);

        while (it.hasCommand()) {
            if (it.isMoveTo()) {
                vc_move_to(path, vc_point { it.x(0) * wh, it.y(0) * wh });
            }
            else if (it.isLineTo()) {
                vc_line_to(path, vc_point { it.x(0) * wh, it.y(0) * wh });
            }
            else if (it.isQuadTo()) {
                vc_quad_to(path, vc_point { it.x(0) * wh, it.y(0) * wh },
                           vc_point { it.x(1) * wh, it.y(1) * wh });
            }
            else if (it.isCubicTo()) {
                vc_cubic_to(path, vc_point { it.x(0) * wh, it.y(0) * wh },
                            vc_point { it.x(1) * wh, it.y(1) * wh },
                            vc_point { it.x(2) * wh, it.y(2) * wh });
            }
            else {
                vc_close(path);
            }

            it.next();
        }

        vc_fill_rule fr = (op == RenderOp::kFillEvenOdd ? vc_fill_rule::EvenOdd : vc_fill_rule::Winding);

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            BLPoint base(_rndCoord.nextPoint(bounds));
            vc_rect base_rect = { (float) base.x, (float) base.y, (float) (base.x + wh), (float) (base.y + wh)};
            vc_transform t = vc_transform_translate(base.x, base.y);
            vc_transform inv_t = vc_transform_translate(-base.x, -base.y);
            vc_paint paint = convert_style(base_rect, style, inv_t);

            vc_set_transform(context, t);
            vc_set_paint(context, paint);
            vc_set_fill_rule(context, fr);

            if (op == RenderOp::kStroke) {
                vc_set_stroke(context, stroke);
                vc_stroke_path(context, path);
            }   else {
                vc_fill_path(context, path);
            }
        }

        vc_path_destroy(path);

        vc_render_to_pixmap(pixmap, context);
    }

    vc_color VelloCpuModule::gen_color() {
        auto bl_color = _rndColor.nextRgba32();
        vc_color color = {(uint8_t) bl_color.r(), (uint8_t) bl_color.g(), (uint8_t) bl_color.b(), (uint8_t) bl_color.a()};

        return color;
    }

    inline vc_paint VelloCpuModule::convert_style(const vc_rect& rect, StyleKind style, vc_transform t) {
//        double w = rect.x1 - rect.x0;
//        double h = rect.y1 - rect.y0;

        vc_color color = gen_color();

        vc_paint paint;
        paint.tag = vc_paint::Tag::Color;
        paint.color = vc_paint::Color_Body{ color };
        return paint;
    }

    vc_rect VelloCpuModule::convert_rect(BLRect bl_rect) {
        return {bl_rect.x, bl_rect.y, (bl_rect.x + bl_rect.w),  (bl_rect.y + bl_rect.h)};
    }

    vc_point VelloCpuModule::convert_point(BLPoint point) {
        return { point.x, point.y };
    }

    vc_rect VelloCpuModule::convert_rect_i(BLRectI bl_rect) {
        return {(float) bl_rect.x, (float) bl_rect.y, (float) (bl_rect.x + bl_rect.w), (float) (bl_rect.y + bl_rect.h)};
    }

    Backend* createVelloCpuBackend() {
        return new VelloCpuModule();
    }
}

#endif // BLEND2D_APPS_ENABLE_VELLO_CPU
