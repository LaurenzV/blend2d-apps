#ifdef BLEND2D_APPS_ENABLE_RAQOTE

#include "backend_raqote.h"
#include "app.h"
#include <vector>

namespace blbench {
    struct RaqoteModule : public Backend {
        rq_draw_target* draw_target {};
        rq_stroke_style stroke;
        rq_image* sprite_images[kBenchNumSprites] {};

        RaqoteModule();
        ~RaqoteModule() override;

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

        rq_color gen_color();
        rq_rect convert_rect(BLRect rect);
        rq_rect convert_rect_i(BLRectI rect);
        rq_point convert_point(BLPoint point);

        rq_blend_mode toRaqoteOperator(uint32_t compOp);
        rq_draw_options createDrawOptions(rq_blend_mode blendMode);
        rq_paint convertStyle(const rq_rect& rect, StyleKind style, rq_transform transform);
    };

    RaqoteModule::RaqoteModule() {
        strcpy(_name, "raqote");
    }

    RaqoteModule::~RaqoteModule() {}

    bool RaqoteModule::supportsCompOp(BLCompOp compOp) const {
        return compOp == BL_COMP_OP_SRC_OVER || compOp == BL_COMP_OP_SRC_COPY ||
               compOp == BL_COMP_OP_DST_OVER || compOp == BL_COMP_OP_SRC_IN ||
               compOp == BL_COMP_OP_DST_IN || compOp == BL_COMP_OP_SRC_OUT ||
               compOp == BL_COMP_OP_DST_OUT || compOp == BL_COMP_OP_SRC_ATOP ||
               compOp == BL_COMP_OP_DST_ATOP || compOp == BL_COMP_OP_XOR ||
               compOp == BL_COMP_OP_PLUS || compOp == BL_COMP_OP_MULTIPLY ||
               compOp == BL_COMP_OP_SCREEN || compOp == BL_COMP_OP_OVERLAY ||
               compOp == BL_COMP_OP_DARKEN || compOp == BL_COMP_OP_LIGHTEN ||
               compOp == BL_COMP_OP_COLOR_DODGE || compOp == BL_COMP_OP_COLOR_BURN ||
               compOp == BL_COMP_OP_HARD_LIGHT || compOp == BL_COMP_OP_SOFT_LIGHT ||
               compOp == BL_COMP_OP_DIFFERENCE || compOp == BL_COMP_OP_EXCLUSION;
    }

    bool RaqoteModule::supportsStyle(StyleKind style) const {
        // Support solid colors, linear gradients, radial gradients, and patterns
        return style == StyleKind::kSolid ||
               style == StyleKind::kLinearPad || 
               style == StyleKind::kLinearRepeat || 
               style == StyleKind::kLinearReflect ||
               style == StyleKind::kRadialPad || 
               style == StyleKind::kRadialRepeat || 
               style == StyleKind::kRadialReflect ||
               style == StyleKind::kPatternNN || 
               style == StyleKind::kPatternBI;
    }

    void RaqoteModule::beforeRun() {
        int w = int(_params.screenW);
        int h = int(_params.screenH);

        draw_target = rq_draw_target_create(w, h);
        
        stroke.width = (float)_params.strokeWidth;
        stroke.cap = rq_cap_style::Butt;
        stroke.join = rq_join_style::Miter;
        stroke.miter_limit = 4.0f;
        stroke.dash_array = nullptr;
        stroke.dash_array_length = 0;
        stroke.dash_offset = 0.0f;
        
        // Initialize sprite images
        for (uint32_t i = 0; i < kBenchNumSprites; i++) {
            const BLImage& sprite = _sprites[i];
            BLImageData spriteData;
            sprite.getData(&spriteData);
            
            // Convert BGRA to ARGB for raqote (it expects ARGB format)
            uint32_t pixel_count = spriteData.size.w * spriteData.size.h;
            std::vector<uint32_t> argb_data(pixel_count);
            const uint8_t* bgra_data = static_cast<const uint8_t*>(spriteData.pixelData);
            
            for (uint32_t j = 0; j < pixel_count; j++) {
                uint8_t b = bgra_data[j * 4 + 0];
                uint8_t g = bgra_data[j * 4 + 1];
                uint8_t r = bgra_data[j * 4 + 2];
                uint8_t a = bgra_data[j * 4 + 3];
                
                argb_data[j] = (a << 24) | (r << 16) | (g << 8) | b;
            }
            
            sprite_images[i] = rq_image_create(
                spriteData.size.w,
                spriteData.size.h,
                argb_data.data()
            );
        }
    }

    void RaqoteModule::afterRun() {
        int w = int(_params.screenW);
        int h = int(_params.screenH);

        BLImageData dstData;
        _surface.create(w, h, BL_FORMAT_PRGB32);
        _surface.makeMutable(&dstData);

        rq_argb *data = rq_draw_target_get_data(draw_target);
        const uint8_t *bytes = rq_argb_data(data);

        memcpy(
            static_cast<uint8_t*>(dstData.pixelData),
            bytes,
            w * h * 4);
        
        rq_argb_destroy(data);
        rq_draw_target_destroy(draw_target);
        
        // Clean up sprite images
        for (uint32_t i = 0; i < kBenchNumSprites; i++) {
            if (sprite_images[i]) {
                rq_image_destroy(sprite_images[i]);
                sprite_images[i] = nullptr;
            }
        }
    }

    void RaqoteModule::flush() {
        
    }

    void RaqoteModule::renderRectA(RenderOp op) {
        rq_transform t = rq_transform_identity();
        BLSizeI bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        int wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            rq_rect rect = convert_rect_i(_rndCoord.nextRectI(bounds, wh, wh));
            rq_draw_options options = createDrawOptions(toRaqoteOperator(_params.compOp));
            rq_draw_target_set_transform(draw_target, t);

            rq_paint paint = convertStyle(rect, style, t);
            if (op == RenderOp::kStroke) {
                rq_draw_target_stroke_rect(draw_target, rect, paint, &stroke, &options);
            } else {
                rq_draw_target_fill_rect(draw_target, rect, paint, &options);
            }
            rq_paint_destroy(paint);
        }
    }

    void RaqoteModule::renderRectF(RenderOp op) {
        rq_transform t = rq_transform_identity();
        BLSizeI bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        int wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            rq_rect rect = convert_rect(_rndCoord.nextRect(bounds, wh, wh));
            rq_draw_options options = createDrawOptions(toRaqoteOperator(_params.compOp));
            rq_draw_target_set_transform(draw_target, t);

            rq_paint paint = convertStyle(rect, style, t);
            if (op == RenderOp::kStroke) {
                rq_draw_target_stroke_rect(draw_target, rect, paint, &stroke, &options);
            } else {
                rq_draw_target_fill_rect(draw_target, rect, paint, &options);
            }
            rq_paint_destroy(paint);
        }
    }

    void RaqoteModule::renderRectRotated(RenderOp op) {
        BLSizeI bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        int wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            BLRect base(_rndCoord.nextRect(bounds, wh, wh));
            rq_rect rect = {0.0f, 0.0f, (float)base.w, (float)base.h};
            rq_draw_options options = createDrawOptions(toRaqoteOperator(_params.compOp));

            // Create rotation transform
            rq_transform translate1 = rq_transform_translate((float)base.x + (float)base.w * 0.5f, (float)base.y + (float)base.h * 0.5f);
            rq_transform rotate = rq_transform_rotate(_rndExtra.nextDouble(0.0, 6.28));
            rq_transform translate2 = rq_transform_translate(-(float)base.w * 0.5f, -(float)base.h * 0.5f);
            rq_transform t = rq_transform_multiply(rq_transform_multiply(translate1, rotate), translate2);

            rq_draw_target_set_transform(draw_target, t);

            rq_paint paint = convertStyle(rect, style, t);
            if (op == RenderOp::kStroke) {
                rq_draw_target_stroke_rect(draw_target, rect, paint, &stroke, &options);
            } else {
                rq_draw_target_fill_rect(draw_target, rect, paint, &options);
            }
            rq_paint_destroy(paint);
        }
    }

    void RaqoteModule::renderRoundF(RenderOp op) {
        BLSizeI bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        int wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            double radius = _rndExtra.nextDouble(4.0, 40.0);
            BLRect base(_rndCoord.nextRect(bounds, wh, wh));
            rq_draw_options options = createDrawOptions(toRaqoteOperator(_params.compOp));

            // Use the new rounded rectangle function
            rq_rect rect = convert_rect(base);
            rq_path *path = rq_rounded_rect(rect, (float)radius, (float)radius);
            
            rq_transform t = rq_transform_identity();
            rq_draw_target_set_transform(draw_target, t);

            rq_paint paint = convertStyle(rect, style, t);
            if (op == RenderOp::kStroke) {
                rq_draw_target_stroke_path(draw_target, path, paint, &stroke, &options);
            } else {
                rq_fill_rule fr = (op == RenderOp::kFillEvenOdd ? rq_fill_rule::EvenOdd : rq_fill_rule::Winding);
                rq_draw_target_fill_path(draw_target, path, paint, fr, &options);
            }
            rq_paint_destroy(paint);

            rq_path_destroy(path);
        }
    }

    void RaqoteModule::renderRoundRotated(RenderOp op) {
        BLSizeI bounds(_params.screenW, _params.screenH);
        StyleKind style = _params.style;
        int wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            double radius = _rndExtra.nextDouble(4.0, 40.0);
            BLRect base(_rndCoord.nextRect(bounds, wh, wh));
            rq_draw_options options = createDrawOptions(toRaqoteOperator(_params.compOp));

            // Create rounded rectangle path at origin
            rq_rect rect_at_origin = {0.0f, 0.0f, (float)base.w, (float)base.h};
            rq_path *path = rq_rounded_rect(rect_at_origin, (float)radius, (float)radius);

            // Create rotation transform
            float w = (float)base.w;
            float h = (float)base.h;
            rq_transform translate1 = rq_transform_translate((float)base.x + w * 0.5f, (float)base.y + h * 0.5f);
            rq_transform rotate = rq_transform_rotate(_rndExtra.nextDouble(0.0, 6.28));
            rq_transform translate2 = rq_transform_translate(-w * 0.5f, -h * 0.5f);
            rq_transform t = rq_transform_multiply(rq_transform_multiply(translate1, rotate), translate2);

            rq_draw_target_set_transform(draw_target, t);

            rq_paint paint = convertStyle(rect_at_origin, style, t);
            if (op == RenderOp::kStroke) {
                rq_draw_target_stroke_path(draw_target, path, paint, &stroke, &options);
            } else {
                rq_fill_rule fr = (op == RenderOp::kFillEvenOdd ? rq_fill_rule::EvenOdd : rq_fill_rule::Winding);
                rq_draw_target_fill_path(draw_target, path, paint, fr, &options);
            }
            rq_paint_destroy(paint);

            rq_path_destroy(path);
        }
    }

    void RaqoteModule::renderPolygon(RenderOp op, uint32_t complexity) {
        BLSizeI bounds(_params.screenW - _params.shapeSize, _params.screenH - _params.shapeSize);
        StyleKind style = _params.style;
        int wh = _params.shapeSize;

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            BLPoint base(_rndCoord.nextPoint(bounds));
            rq_draw_options options = createDrawOptions(toRaqoteOperator(_params.compOp));

            double x = _rndCoord.nextDouble(base.x, base.x + wh);
            double y = _rndCoord.nextDouble(base.y, base.y + wh);

            rq_path_builder *builder = rq_path_builder_create();
            rq_path_builder_move_to(builder, (float)x, (float)y);
            
            for (uint32_t p = 1; p < complexity; p++) {
                x = _rndCoord.nextDouble(base.x, base.x + wh);
                y = _rndCoord.nextDouble(base.y, base.y + wh);
                rq_path_builder_line_to(builder, (float)x, (float)y);
            }

            rq_path_builder_close(builder);
            rq_path *path = rq_path_builder_finish(builder);

            rq_transform t = rq_transform_identity();
            rq_draw_target_set_transform(draw_target, t);

            rq_rect base_rect = {(float)base.x, (float)base.y, (float)wh, (float)wh};
            rq_paint paint = convertStyle(base_rect, style, t);
            if (op == RenderOp::kStroke) {
                rq_draw_target_stroke_path(draw_target, path, paint, &stroke, &options);
            } else {
                rq_fill_rule fr = (op == RenderOp::kFillEvenOdd ? rq_fill_rule::EvenOdd : rq_fill_rule::Winding);
                rq_draw_target_fill_path(draw_target, path, paint, fr, &options);
            }
            rq_paint_destroy(paint);

            rq_path_destroy(path);
        }
    }

    void RaqoteModule::renderShape(RenderOp op, ShapeData shape) {
        BLSizeI bounds(_params.screenW - _params.shapeSize, _params.screenH - _params.shapeSize);
        StyleKind style = _params.style;
        double wh = double(_params.shapeSize);

        rq_path_builder *builder = rq_path_builder_create();
        ShapeIterator it(shape);

        while (it.hasCommand()) {
            if (it.isMoveTo()) {
                rq_path_builder_move_to(builder, (float)(it.x(0) * wh), (float)(it.y(0) * wh));
            }
            else if (it.isLineTo()) {
                rq_path_builder_line_to(builder, (float)(it.x(0) * wh), (float)(it.y(0) * wh));
            }
            else if (it.isQuadTo()) {
                rq_path_builder_quad_to(builder, (float)(it.x(0) * wh), (float)(it.y(0) * wh),
                                       (float)(it.x(1) * wh), (float)(it.y(1) * wh));
            }
            else if (it.isCubicTo()) {
                rq_path_builder_cubic_to(builder, (float)(it.x(0) * wh), (float)(it.y(0) * wh),
                                        (float)(it.x(1) * wh), (float)(it.y(1) * wh),
                                        (float)(it.x(2) * wh), (float)(it.y(2) * wh));
            }
            else {
                rq_path_builder_close(builder);
            }

            it.next();
        }

        rq_path *path = rq_path_builder_finish(builder);

        for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
            BLPoint base(_rndCoord.nextPoint(bounds));
            rq_rect base_rect = {(float)base.x, (float)base.y, (float)wh, (float)wh};
            rq_draw_options options = createDrawOptions(toRaqoteOperator(_params.compOp));

            rq_transform t = rq_transform_translate((float)base.x, (float)base.y);
            rq_draw_target_set_transform(draw_target, t);

            rq_paint paint = convertStyle(base_rect, style, t);
            if (op == RenderOp::kStroke) {
                rq_draw_target_stroke_path(draw_target, path, paint, &stroke, &options);
            } else {
                rq_fill_rule fr = (op == RenderOp::kFillEvenOdd ? rq_fill_rule::EvenOdd : rq_fill_rule::Winding);
                rq_draw_target_fill_path(draw_target, path, paint, fr, &options);
            }
            rq_paint_destroy(paint);
        }

        rq_path_destroy(path);
    }

    rq_color RaqoteModule::gen_color() {
        BLRgba32 bl_color = _rndColor.nextRgba32();
        return {(uint8_t)bl_color.r(), (uint8_t)bl_color.g(), (uint8_t)bl_color.b(), (uint8_t)bl_color.a()};
    }

    rq_rect RaqoteModule::convert_rect(BLRect bl_rect) {
        return {(float)bl_rect.x, (float)bl_rect.y, (float)bl_rect.w, (float)bl_rect.h};
    }

    rq_point RaqoteModule::convert_point(BLPoint point) {
        return {(float)point.x, (float)point.y};
    }

    rq_rect RaqoteModule::convert_rect_i(BLRectI bl_rect) {
        return {(float)bl_rect.x, (float)bl_rect.y, (float)bl_rect.w, (float)bl_rect.h};
    }

    rq_blend_mode RaqoteModule::toRaqoteOperator(uint32_t compOp) {
        switch (compOp) {
            case BL_COMP_OP_SRC_OVER:
                return rq_blend_mode::SourceOver;
            case BL_COMP_OP_SRC_COPY:
                return rq_blend_mode::SourceCopy;
            case BL_COMP_OP_CLEAR:
                return rq_blend_mode::Clear;
            case BL_COMP_OP_DST_COPY:
                return rq_blend_mode::Destination;
            case BL_COMP_OP_DST_OVER:
                return rq_blend_mode::DestinationOver;
            case BL_COMP_OP_SRC_IN:
                return rq_blend_mode::SourceIn;
            case BL_COMP_OP_DST_IN:
                return rq_blend_mode::DestinationIn;
            case BL_COMP_OP_SRC_OUT:
                return rq_blend_mode::SourceOut;
            case BL_COMP_OP_DST_OUT:
                return rq_blend_mode::DestinationOut;
            case BL_COMP_OP_SRC_ATOP:
                return rq_blend_mode::SourceAtop;
            case BL_COMP_OP_DST_ATOP:
                return rq_blend_mode::DestinationAtop;
            case BL_COMP_OP_XOR:
                return rq_blend_mode::Xor;
            case BL_COMP_OP_PLUS:
                return rq_blend_mode::Add;
            case BL_COMP_OP_MULTIPLY:
                return rq_blend_mode::Multiply;
            case BL_COMP_OP_SCREEN:
                return rq_blend_mode::Screen;
            case BL_COMP_OP_OVERLAY:
                return rq_blend_mode::Overlay;
            case BL_COMP_OP_DARKEN:
                return rq_blend_mode::Darken;
            case BL_COMP_OP_LIGHTEN:
                return rq_blend_mode::Lighten;
            case BL_COMP_OP_COLOR_DODGE:
                return rq_blend_mode::ColorDodge;
            case BL_COMP_OP_COLOR_BURN:
                return rq_blend_mode::ColorBurn;
            case BL_COMP_OP_HARD_LIGHT:
                return rq_blend_mode::HardLight;
            case BL_COMP_OP_SOFT_LIGHT:
                return rq_blend_mode::SoftLight;
            case BL_COMP_OP_DIFFERENCE:
                return rq_blend_mode::Difference;
            case BL_COMP_OP_EXCLUSION:
                return rq_blend_mode::Exclusion;
            default:
                return rq_blend_mode::SourceOver;
        }
    }

    rq_draw_options RaqoteModule::createDrawOptions(rq_blend_mode blendMode) {
        return {1.0f, blendMode};
    }

    rq_paint RaqoteModule::convertStyle(const rq_rect& rect, StyleKind style, rq_transform transform) {
        rq_spread_mode spread_mode;
        
        // Map spread modes
        switch(style) {
            case StyleKind::kLinearReflect:
            case StyleKind::kRadialReflect:
                spread_mode = rq_spread_mode::Reflect;
                break;
            case StyleKind::kLinearRepeat:
            case StyleKind::kRadialRepeat:
            case StyleKind::kPatternNN:
            case StyleKind::kPatternBI:
                spread_mode = rq_spread_mode::Repeat;
                break;
            default:
                spread_mode = rq_spread_mode::Pad;
        }

        switch (style) {
            case StyleKind::kSolid: {
                rq_color color = gen_color();
                rq_paint paint;
                paint.tag = rq_paint::Tag::Solid;
                paint.solid = rq_paint::Solid_Body{color};
                return paint;
            }
            case StyleKind::kLinearPad:
            case StyleKind::kLinearRepeat:
            case StyleKind::kLinearReflect: {
                rq_color c0 = gen_color();
                rq_color c1 = gen_color();
                rq_color c2 = gen_color();

                float w = rect.width;
                float h = rect.height;
                float x0 = rect.x + w * 0.2f;
                float y0 = rect.y + h * 0.2f;
                float x1 = rect.x + w * 0.8f;
                float y1 = rect.y + h * 0.8f;

                rq_linear_gradient *grad = rq_linear_gradient_create(x0, y0, x1, y1, spread_mode, transform);
                
                rq_gradient_stop stop0 = {0.0f, c0};
                rq_gradient_stop stop1 = {0.5f, c1};
                rq_gradient_stop stop2 = {1.0f, c2};
                rq_linear_gradient_add_stop(grad, stop0);
                rq_linear_gradient_add_stop(grad, stop1);
                rq_linear_gradient_add_stop(grad, stop2);

                rq_paint paint;
                paint.tag = rq_paint::Tag::LinearGradient;
                paint.linear_gradient = rq_paint::LinearGradient_Body{grad};
                return paint;
            }
            case StyleKind::kRadialPad:
            case StyleKind::kRadialRepeat:
            case StyleKind::kRadialReflect: {
                rq_color c0 = gen_color();
                rq_color c1 = gen_color();
                rq_color c2 = gen_color();

                float w = rect.width;
                float h = rect.height;
                float center_x = rect.x + (w / 2.0f);
                float center_y = rect.y + (h / 2.0f);
                float radius = (w + h) / 4.0f;
                float inner_x = center_x - radius / 2.0f;
                float inner_y = center_y - radius / 2.0f;

                rq_radial_gradient *grad = rq_radial_gradient_create(
                    inner_x, inner_y, 0.0f,  // inner circle (radius 0)
                    center_x, center_y, radius,  // outer circle
                    spread_mode, transform);
                
                rq_gradient_stop stop0 = {0.0f, c0};
                rq_gradient_stop stop1 = {0.5f, c1};
                rq_gradient_stop stop2 = {1.0f, c2};
                rq_radial_gradient_add_stop(grad, stop0);
                rq_radial_gradient_add_stop(grad, stop1);
                rq_radial_gradient_add_stop(grad, stop2);

                rq_paint paint;
                paint.tag = rq_paint::Tag::RadialGradient;
                paint.radial_gradient = rq_paint::RadialGradient_Body{grad};
                return paint;
            }
            case StyleKind::kPatternNN:
            case StyleKind::kPatternBI: {
                uint32_t spriteId = nextSpriteId();
                rq_filter_mode filter = (style == StyleKind::kPatternNN) ? 
                    rq_filter_mode::Nearest : rq_filter_mode::Bilinear;
                
                rq_transform pattern_transform = rq_transform_multiply(
                    rq_transform_translate(rect.x, rect.y), transform);
                    
                rq_pattern *pattern = rq_pattern_create(
                    sprite_images[spriteId],
                    rq_extend_mode::Repeat,
                    filter,
                    pattern_transform
                );

                rq_paint paint;
                paint.tag = rq_paint::Tag::Pattern;
                paint.pattern = rq_paint::Pattern_Body{pattern};
                return paint;
            }
            default: {
                // Default to solid color
                rq_color color = gen_color();
                rq_paint paint;
                paint.tag = rq_paint::Tag::Solid;
                paint.solid = rq_paint::Solid_Body{color};
                return paint;
            }
        }
    }

    Backend* createRaqoteBackend() {
        return new RaqoteModule();
    }

} // {blbench}

#endif // BLEND2D_APPS_ENABLE_RAQOTE