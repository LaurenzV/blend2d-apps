#include "backend_cpu_sparse.h"

namespace blbench {
    struct CpuSparseModule : public Backend {
        sp_context* context {};
        sp_pixmap* pixmap {};
        sp_stroke stroke;

        CpuSparseModule();
        ~CpuSparseModule() override;

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

        inline sp_paint convert_style(const sp_rect& rect, StyleKind style, sp_transform t);
        sp_color gen_color();
        sp_rect convert_rect(BLRect rect);
        sp_rect convert_rect_i(BLRectI rect);
        sp_point convert_point(BLPoint rect);
    };

    CpuSparseModule::CpuSparseModule() {
        strcpy(_name, "cpu-sparse");
    }

    CpuSparseModule::~CpuSparseModule() {}

    bool CpuSparseModule::supportsCompOp(BLCompOp compOp) const {
        return compOp == BL_COMP_OP_SRC_OVER;
    }

    bool CpuSparseModule::supportsStyle(StyleKind style) const {
        return style <= StyleKind::kSolid;
    }

    void CpuSparseModule::beforeRun() {
        int w = int(_params.screenW);
        int h = int(_params.screenH);

        pixmap = sp_pixmap_create(w, h);
        context = sp_context_create(w, h);
        stroke = sp_stroke { _params.strokeWidth };
    }

    void CpuSparseModule::afterRun() {
        int w = int(_params.screenW);
        int h = int(_params.screenH);

        BLImageData dstData;
        _surface.create(int(w), int(h), BL_FORMAT_PRGB32);
        _surface.makeMutable(&dstData);

        auto data = sp_data(pixmap);
        auto bytes = sp_argb_data(data);

        memcpy(
                static_cast<uint8_t*>(dstData.pixelData),
                bytes,
                w * h * 4);
        sp_argb_destroy(data);
        sp_pixmap_destroy(pixmap);
        sp_context_destroy(context);
    }

    void CpuSparseModule::flush() {

    }

    void CpuSparseModule::renderRectA(RenderOp op) {
        sp_transform t = sp_transform_identity();
        BLSizeI bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        int wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            sp_rect rect = convert_rect_i(_rndCoord.nextRectI(bounds, wh, wh));
            sp_paint paint = convert_style(rect, style, t);

            if (op == RenderOp::kStroke) {
                sp_stroke_rect(context, rect, paint, stroke);
            }   else {
                sp_fill_rect(context, rect, paint);
            }
        }

        sp_render_to_pixmap(pixmap, context);
    }

    void CpuSparseModule::renderRectF(RenderOp op) {
        sp_transform t = sp_transform_identity();
        BLSizeI bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        int wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            sp_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));
            sp_paint paint = convert_style(rect, style, t);

            if (op == RenderOp::kStroke) {
                sp_stroke_rect(context, rect, paint, stroke);
            }   else {
                sp_fill_rect(context, rect, paint);
            }
        }

        sp_render_to_pixmap(pixmap, context);
    }

    void CpuSparseModule::renderRectRotated(RenderOp op) {
        BLSize bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        sp_transform id = sp_transform_identity();

        double cx = double(_params.screenW) * 0.5;
        double cy = double(_params.screenH) * 0.5;
        double wh = _params.shapeSize;
        double angle = 0.0;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
            sp_transform t = sp_transform_rotate_at(angle, cx, cy);
            sp_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));
            sp_paint paint = convert_style(rect, style, id);

            sp_set_transform(context, t);

            if (op == RenderOp::kStroke) {
                sp_stroke_rect(context, rect, paint, stroke);
            }   else {
                sp_fill_rect(context, rect, paint);
            }

        }

        sp_render_to_pixmap(pixmap, context);
    }

    void CpuSparseModule::renderRoundF(RenderOp op) {
        BLSize bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        sp_transform t = sp_transform_identity();
        double wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            double radius = _rndExtra.nextDouble(4.0, 40.0);
            sp_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));
            sp_paint paint = convert_style(rect, style, t);

            sp_path *p = sp_rounded_rect(rect, radius);

            if (op == RenderOp::kStroke) {
                sp_stroke_path(context, p, paint, stroke);
            }   else {
                sp_fill_path(context, p, paint, sp_fill_rule::Winding);
            }

            sp_path_destroy(p);
        }

        sp_render_to_pixmap(pixmap, context);
    }

    void CpuSparseModule::renderRoundRotated(RenderOp op) {
        BLSize bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        sp_transform t = sp_transform_identity();

        double cx = double(_params.screenW) * 0.5;
        double cy = double(_params.screenH) * 0.5;
        double wh = _params.shapeSize;
        double angle = 0.0;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
            sp_transform t = sp_transform_rotate_at(angle, cx, cy);
            double radius = _rndExtra.nextDouble(4.0, 40.0);
            sp_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));
            sp_paint paint = convert_style(rect, style, t);

            sp_path *p = sp_rounded_rect(rect, radius);

            sp_set_transform(context, t);

            if (op == RenderOp::kStroke) {
                sp_stroke_path(context, p, paint, stroke);
            }   else {
                sp_fill_path(context, p, paint, sp_fill_rule::Winding);
            }

            sp_path_destroy(p);
        }

        sp_render_to_pixmap(pixmap, context);
    }

    void CpuSparseModule::renderPolygon(RenderOp op, uint32_t complexity) {
        BLSizeI bounds(_params.screenW - _params.shapeSize,
                       _params.screenH - _params.shapeSize);
        sp_transform t = sp_transform_identity();
        float wh = (float) _params.shapeSize;
        StyleKind style = _params.style;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            sp_point base = convert_point(_rndCoord.nextPoint(bounds));
            sp_rect base_rect = {base.x, base.y, base.x + wh, base.y + wh};
            sp_paint paint = convert_style(base_rect, style, t);

            double x = _rndCoord.nextDouble(base.x, base.x + wh);
            double y = _rndCoord.nextDouble(base.y, base.y + wh);

            sp_path *path = sp_path_create();
            sp_move_to(path, sp_point {x, y});
            for (uint32_t p = 1; p < complexity; p++) {
                x = _rndCoord.nextDouble(base.x, base.x + wh);
                y = _rndCoord.nextDouble(base.y, base.y + wh);
                sp_line_to(path, sp_point {x, y});
            }

            sp_close(path);

            sp_fill_rule fr = (op == RenderOp::kFillEvenOdd ? sp_fill_rule::EvenOdd : sp_fill_rule::Winding);

            if (op == RenderOp::kStroke) {
                sp_stroke_path(context, path, paint, stroke);
            }   else {
                sp_fill_path(context, path, paint, fr);
            }

            sp_path_destroy(path);
        }

        sp_render_to_pixmap(pixmap, context);
    }

    void CpuSparseModule::renderShape(RenderOp op, ShapeData shape) {
        BLSizeI bounds(_params.screenW - _params.shapeSize, _params.screenH - _params.shapeSize);
        StyleKind style = _params.style;
        double wh = double(_params.shapeSize);

        sp_path *path = sp_path_create();
        ShapeIterator it(shape);

        while (it.hasCommand()) {
            if (it.isMoveTo()) {
                sp_move_to(path, sp_point { it.x(0) * wh, it.y(0) * wh });
            }
            else if (it.isLineTo()) {
                sp_line_to(path, sp_point { it.x(0) * wh, it.y(0) * wh });
            }
            else if (it.isQuadTo()) {
                sp_quad_to(path, sp_point { it.x(0) * wh, it.y(0) * wh },
                           sp_point { it.x(1) * wh, it.y(1) * wh });
            }
            else if (it.isCubicTo()) {
                sp_cubic_to(path, sp_point { it.x(0) * wh, it.y(0) * wh },
                            sp_point { it.x(1) * wh, it.y(1) * wh },
                            sp_point { it.x(2) * wh, it.y(2) * wh });
            }
            else {
                sp_close(path);
            }

            it.next();
        }

        sp_fill_rule fr = (op == RenderOp::kFillEvenOdd ? sp_fill_rule::EvenOdd : sp_fill_rule::Winding);

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            BLPoint base(_rndCoord.nextPoint(bounds));
            sp_rect base_rect = { (float) base.x, (float) base.y, (float) (base.x + wh), (float) (base.y + wh)};
            sp_transform t = sp_transform_translate(base.x, base.y);
            sp_transform inv_t = sp_transform_translate(-base.x, -base.y);
            sp_paint paint = convert_style(base_rect, style, inv_t);

            sp_set_transform(context, t);

            if (op == RenderOp::kStroke) {
                sp_stroke_path(context, path, paint, stroke);
            }   else {
                sp_fill_path(context, path, paint, fr);
            }

//            sp_paint_destroy(paint);
        }

        sp_path_destroy(path);

        sp_render_to_pixmap(pixmap, context);
    }

    sp_color CpuSparseModule::gen_color() {
        auto bl_color = _rndColor.nextRgba32();
        sp_color color = {(uint8_t) bl_color.r(), (uint8_t) bl_color.g(), (uint8_t) bl_color.b(), (uint8_t) bl_color.a()};

        return color;
    }

    inline sp_paint CpuSparseModule::convert_style(const sp_rect& rect, StyleKind style, sp_transform t) {
//        double w = rect.x1 - rect.x0;
//        double h = rect.y1 - rect.y0;

        sp_color color = gen_color();

        sp_paint paint;
        paint.tag = sp_paint::Tag::Color;
        paint.color = sp_paint::Color_Body{ color };
        return paint;
    }

    sp_rect CpuSparseModule::convert_rect(BLRect bl_rect) {
        return {bl_rect.x, bl_rect.y, (bl_rect.x + bl_rect.w),  (bl_rect.y + bl_rect.h)};
    }

    sp_point CpuSparseModule::convert_point(BLPoint point) {
        return { point.x, point.y };
    }

    sp_rect CpuSparseModule::convert_rect_i(BLRectI bl_rect) {
        return {(float) bl_rect.x, (float) bl_rect.y, (float) (bl_rect.x + bl_rect.w), (float) (bl_rect.y + bl_rect.h)};
    }

    Backend* createCpuSparseBackend() {
        return new CpuSparseModule();
    }
}

