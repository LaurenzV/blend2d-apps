#include "backend_tiny_skia.h"

namespace blbench {
    struct TinySkiaModule : public Backend {
        ts_pixmap* pixmap {};

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

        ts_color next_color();

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
                ts_color color = next_color();

                auto bs_rect = _rndCoord.nextRectI(bounds, wh, wh);
                ts_rect rect = {(float) bs_rect.x, (float) bs_rect.y, (float) (bs_rect.x + bs_rect.w), (float) (bs_rect.y + bs_rect.h)};

                ts_pixmap_fill_rect(pixmap, rect, t, color, toTinySkiaOperator(_params.compOp));
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
                ts_color color = next_color();

                auto bs_rect = _rndCoord.nextRect(bounds, wh, wh);
                ts_rect rect = {(float) bs_rect.x, (float) bs_rect.y, (float) (bs_rect.x + bs_rect.w), (float) (bs_rect.y + bs_rect.h)};

                ts_pixmap_fill_rect(pixmap, rect, t, color, toTinySkiaOperator(_params.compOp));
            }
        }
    }
    void TinySkiaModule::renderRectRotated(RenderOp op) {
    }
    void TinySkiaModule::renderRoundF(RenderOp op) {
    }
    void TinySkiaModule::renderRoundRotated(RenderOp op) {
    }
    void TinySkiaModule::renderPolygon(RenderOp op, uint32_t complexity) {
    }
    void TinySkiaModule::renderShape(RenderOp op, ShapeData shape) {
    }

    ts_color TinySkiaModule::next_color() {
        auto bl_color = _rndColor.nextRgba32();
        return {(uint8_t) bl_color.r(), (uint8_t) bl_color.g(), (uint8_t) bl_color.b(), (uint8_t) bl_color.a()};

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

