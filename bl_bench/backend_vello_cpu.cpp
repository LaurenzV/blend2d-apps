#ifdef BLEND2D_APPS_ENABLE_VELLO_CPU

#include "backend_vello_cpu.h"
#include "app.h"
#include <algorithm>

namespace blbench {
    struct VelloCpuModule : public Backend {
        vc_context* context {};
        vc_pixmap* pixmap {};
        vc_stroke stroke;
        vc_arc_pixmap* sprite_pixmaps[kBenchNumSprites] {};

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
        void set_paint_transform(vc_transform t);
        vc_color gen_color();
        vc_rect convert_rect(BLRect rect);
        vc_rect convert_rect_i(BLRectI rect);

        void convertBgraToRgba(const uint8_t* bgra_data, uint8_t* rgba_data, uint32_t pixel_count);

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
        return style <= StyleKind::kPatternBI;
    }

    void VelloCpuModule::beforeRun() {
        int w = int(_params.screenW);
        int h = int(_params.screenH);

        pixmap = vc_pixmap_create(w, h);
        context = vc_context_create(w, h);
        stroke = vc_stroke { _params.strokeWidth };
        
        for (uint32_t i = 0; i < kBenchNumSprites; i++) {
            const BLImage& sprite = _sprites[i];
            BLImageData spriteData;
            sprite.getData(&spriteData);
            
            uint32_t pixel_count = spriteData.size.w * spriteData.size.h;
            uint8_t* rgba_data = new uint8_t[pixel_count * 4];
            convertBgraToRgba(static_cast<const uint8_t*>(spriteData.pixelData), rgba_data, pixel_count);
            
            sprite_pixmaps[i] = vc_pixmap_from_data(
                rgba_data,
                spriteData.size.w,
                spriteData.size.h
            );
            
            delete[] rgba_data;
        }
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
        
        for (uint32_t i = 0; i < kBenchNumSprites; i++) {
            if (sprite_pixmaps[i]) {
                vc_arc_pixmap_destroy(sprite_pixmaps[i]);
                sprite_pixmaps[i] = nullptr;
            }
        }
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
            set_paint_transform(t);
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
            set_paint_transform(t);
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
            set_paint_transform(id);
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
            set_paint_transform(t);
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
            set_paint_transform(vc_transform_identity());
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
            set_paint_transform(t);
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
            set_paint_transform(inv_t);
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
        if (style == StyleKind::kPatternNN || style == StyleKind::kPatternBI) {
            vc_transform rect_transform = vc_transform_translate(rect.x0, rect.y0);
            vc_transform pattern_transform = vc_transform_combine(rect_transform, t);
            set_paint_transform(pattern_transform);
        }
        double w = rect.x1 - rect.x0;
        double h = rect.y1 - rect.y0;
        double cx = rect.x0 + w * 0.5;
        double cy = rect.y0 + h * 0.5;

        vc_paint paint;

        switch (style) {
            case StyleKind::kSolid: {
                vc_color color = gen_color();
                paint.tag = vc_paint::Tag::Color;
                paint.color = vc_paint::Color_Body{color};
                break;
            }
            case StyleKind::kLinearPad:
            case StyleKind::kLinearRepeat:
            case StyleKind::kLinearReflect: {
                vc_extend extend = (style == StyleKind::kLinearPad) ? vc_extend::Pad :
                                 (style == StyleKind::kLinearRepeat) ? vc_extend::Repeat : vc_extend::Reflect;
                
                vc_point start = {rect.x0 + w * 0.2, rect.y0 + h * 0.2};
                vc_point end = {rect.x0 + w * 0.8, rect.y0 + h * 0.8};
                
                vc_linear_gradient* gradient = vc_linear_gradient_create(start, end, extend);
                
                vc_color color0 = gen_color();
                vc_color color1 = gen_color();
                vc_color color2 = gen_color();
                vc_gradient_stop stop0 = {0.0, color0};
                vc_gradient_stop stop1 = {0.5, color1};
                vc_gradient_stop stop2 = {1.0, color2};
                
                vc_linear_gradient_push_stop(gradient, stop0);
                vc_linear_gradient_push_stop(gradient, stop1);
                vc_linear_gradient_push_stop(gradient, stop2);
                
                paint.tag = vc_paint::Tag::LinearGradient;
                paint.linear_gradient = vc_paint::LinearGradient_Body{gradient};
                break;
            }
            case StyleKind::kRadialPad:
            case StyleKind::kRadialRepeat:
            case StyleKind::kRadialReflect: {
                vc_extend extend = (style == StyleKind::kRadialPad) ? vc_extend::Pad :
                                 (style == StyleKind::kRadialRepeat) ? vc_extend::Repeat : vc_extend::Reflect;
                
                double radius = (w + h) / 4.0;
                vc_point center1 = {rect.x0 + w / 2.0, rect.y0 + h / 2.0};
                vc_point center0 = {center1.x - radius / 2.0, center1.y - radius / 2.0};
                double radius0 = 0.0;
                double radius1 = radius;
                
                vc_radial_gradient* gradient = vc_radial_gradient_create(center0, radius0, center1, radius1, extend);

                vc_color color0 = gen_color();
                vc_color color1 = gen_color();
                vc_color color2 = gen_color();
                vc_gradient_stop stop0 = {0.0, color0};
                vc_gradient_stop stop1 = {0.5, color1};
                vc_gradient_stop stop2 = {1.0, color2};
                
                vc_radial_gradient_push_stop(gradient, stop0);
                vc_radial_gradient_push_stop(gradient, stop1);
                vc_radial_gradient_push_stop(gradient, stop2);
                
                paint.tag = vc_paint::Tag::RadialGradient;
                paint.radial_gradient = vc_paint::RadialGradient_Body{gradient};
                break;
            }
            case StyleKind::kConic: {
                vc_point center = {cx, cy};
                double start_angle = 0.0;
                double end_angle = 360.0;
                
                vc_sweep_gradient* gradient = vc_sweep_gradient_create(center, start_angle, end_angle, vc_extend::Pad);

                vc_color color0 = gen_color();
                vc_color color1 = gen_color();
                vc_color color2 = gen_color();
                vc_gradient_stop stop0 = {0.0, color0};
                vc_gradient_stop stop1 = {0.33, color1};
                vc_gradient_stop stop2 = {0.66, color2};
                vc_gradient_stop stop3 = {1.0, color0}; 
                
                vc_sweep_gradient_push_stop(gradient, stop0);
                vc_sweep_gradient_push_stop(gradient, stop1);
                vc_sweep_gradient_push_stop(gradient, stop2);
                vc_sweep_gradient_push_stop(gradient, stop3);
                
                paint.tag = vc_paint::Tag::SweepGradient;
                paint.sweep_gradient = vc_paint::SweepGradient_Body{gradient};
                break;
            }
            case StyleKind::kPatternNN:
            case StyleKind::kPatternBI: {
                uint32_t spriteId = nextSpriteId();
                
                vc_image_quality quality = (style == StyleKind::kPatternNN) ? 
                    vc_image_quality::Low : vc_image_quality::Medium;
                // TODO: DOn't leak!
                vc_image* image = vc_image_create(sprite_pixmaps[spriteId], vc_extend::Repeat, vc_extend::Repeat, quality);
                
                paint.tag = vc_paint::Tag::Image;
                paint.image = vc_paint::Image_Body{image};
                
                break;
            }
            default:
                vc_color color = gen_color();
                paint.tag = vc_paint::Tag::Color;
                paint.color = vc_paint::Color_Body{color};
                break;
        }

        return paint;
    }

    void VelloCpuModule::set_paint_transform(vc_transform t) {
        vc_set_paint_transform(context, t);
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

    void VelloCpuModule::convertBgraToRgba(const uint8_t* bgra_data, uint8_t* rgba_data, uint32_t pixel_count) {
        for (uint32_t i = 0; i < pixel_count; i++) {
            uint32_t offset = i * 4;
            
            rgba_data[offset + 0] = bgra_data[offset + 2]; 
            rgba_data[offset + 1] = bgra_data[offset + 1]; 
            rgba_data[offset + 2] = bgra_data[offset + 0]; 
            rgba_data[offset + 3] = bgra_data[offset + 3]; 
        }
    }
    
    Backend* createVelloCpuBackend() {
        return new VelloCpuModule();
    }
}

#endif // BLEND2D_APPS_ENABLE_VELLO_CPU
