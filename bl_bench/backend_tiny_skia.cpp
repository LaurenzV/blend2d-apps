#include "backend_tiny_skia.h"

namespace blbench {
    struct TinySkiaModule : public Backend {
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
    };

    TinySkiaModule::TinySkiaModule() {
        strcpy(_name, "tiny-skia");
    }

    TinySkiaModule::~TinySkiaModule() {}

    bool TinySkiaModule::supportsCompOp(BLCompOp compOp) const {
        return true;
    }

    bool TinySkiaModule::supportsStyle(StyleKind style) const {
        return true;
    }

    void TinySkiaModule::beforeRun() {

    }

    void TinySkiaModule::afterRun() {

    }

    void TinySkiaModule::flush() {

    }

    void TinySkiaModule::renderRectA(RenderOp op) {
    }
    void TinySkiaModule::renderRectF(RenderOp op) {
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

    Backend* createTinySkiaBackend() {
        return new TinySkiaModule();
    }
}

