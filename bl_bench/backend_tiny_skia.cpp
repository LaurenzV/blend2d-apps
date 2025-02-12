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

        ts_color convert_color(BLRgba32 color);
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
        return style == StyleKind::kSolid;
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

        if (style == StyleKind::kSolid) {
            for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
                ts_color color = convert_color(_rndColor.nextRgba32());
                ts_rect rect = convert_rect_i(_rndCoord.nextRectI(bounds, wh, wh));

                if (op == RenderOp::kStroke) {
                    ts_pixmap_stroke_rect(pixmap, rect, t, color, stroke, toTinySkiaOperator(_params.compOp));
                }   else {
                    ts_pixmap_fill_rect(pixmap, rect, t, color, toTinySkiaOperator(_params.compOp));
                }

            }
        }
    }

    void TinySkiaModule::renderRectF(RenderOp op) {
        ts_transform t = ts_transform_identity();
        BLSizeI bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        int wh = _params.shapeSize;

        if (style == StyleKind::kSolid) {
            for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
                ts_color color = convert_color(_rndColor.nextRgba32());
                ts_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));

                if (op == RenderOp::kStroke) {
                    ts_pixmap_stroke_rect(pixmap, rect, t, color, stroke, toTinySkiaOperator(_params.compOp));
                }   else {
                    ts_pixmap_fill_rect(pixmap, rect, t, color, toTinySkiaOperator(_params.compOp));
                }
            }
        }
    }

    void TinySkiaModule::renderRectRotated(RenderOp op) {
        BLSize bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;

        double cx = double(_params.screenW) * 0.5;
        double cy = double(_params.screenH) * 0.5;
        double wh = _params.shapeSize;
        double angle = 0.0;

        if (style == StyleKind::kSolid) {
            for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
                ts_transform t = ts_transform_rotate_at(angle * 180.0 / 3.141592653, cx, cy);
                ts_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));
                ts_color color = convert_color(_rndColor.nextRgba32());

                if (op == RenderOp::kStroke) {
                    ts_pixmap_fill_rect(pixmap, rect, t, color, toTinySkiaOperator(_params.compOp));
                }   else {
                    ts_pixmap_stroke_rect(pixmap, rect, t, color, stroke, toTinySkiaOperator(_params.compOp));
                }
            }
        }
    }
    void TinySkiaModule::renderRoundF(RenderOp op) {
        // TODO: Placeholder
        renderRectF(op);
    }
    void TinySkiaModule::renderRoundRotated(RenderOp op) {
        // TODO: Placeholder
        renderRectRotated(op);
    }

    void TinySkiaModule::renderPolygon(RenderOp op, uint32_t complexity) {
        BLSizeI bounds(_params.screenW - _params.shapeSize,
                       _params.screenH - _params.shapeSize);
        ts_transform t = ts_transform_identity();
        double wh = double(_params.shapeSize);

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            ts_color color = convert_color(_rndColor.nextRgba32());
            ts_point base = convert_point(_rndCoord.nextPoint(bounds));

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
                ts_pixmap_stroke_path(pixmap, path, t, color, stroke, toTinySkiaOperator(_params.compOp));
            }   else {
                ts_pixmap_fill_path(pixmap, path, t, color, fr, toTinySkiaOperator(_params.compOp));
            }

            ts_path_destroy(path);
        }
    }

    void TinySkiaModule::renderShape(RenderOp op, ShapeData shape) {
        // TODO: Placeholder
        renderRectRotated(op);
    }

    ts_color TinySkiaModule::convert_color(BLRgba32 bl_color) {
        return {(uint8_t) bl_color.r(), (uint8_t) bl_color.g(), (uint8_t) bl_color.b(), (uint8_t) bl_color.a()};
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

