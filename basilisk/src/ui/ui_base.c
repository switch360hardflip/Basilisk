
 /**
  MIT License

  Copyright (c) 2026 switch360hardflip <switch360hardflip@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
  */


 /**
  MIT License

  Copyright (c) 2026 switch360hardflip <switch360hardflip@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
  */

#include <basilisk-mod.h>
#include <bsmod_cache.h>
#include <basilisk.h>

#define BASILISK_TITLE_BAR_HEIGHT 32

//BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, icon)
BSGFX_CACHE_COLOR_MATERIAL(border_outline_color, BS_RGBA(38, 38, 38, 255))
BSGFX_CACHE_COLOR_MATERIAL(background_color, BS_RGBA(52, 52, 52, 255))

void instantiateBaseUI(bs_Context* context, const CommonSubtypes* subtypes) {
    bs_ivec2 resolution = bs_resolution(context);
    bs_vec2 title_bar_size = { resolution.x, BASILISK_TITLE_BAR_HEIGHT };

    bs_vec3 position;
    bsgfx_UIElement element_v;
    bsgfx_UIElement* element = &element_v;

    // title icon
    position = BS_V3(0, resolution.y - title_bar_size.y, 0);
    position.y = 0.0;

   
    const int padding = context->border_padding;
    const int border_outline_size = 1;

   /**
    Border outline
    */
    bs_vec3 border_position = { padding, padding, position.z };
    bs_vec2 border_size = { resolution.x - padding * 2, resolution.y - BASILISK_TITLE_BAR_HEIGHT - padding };
    bsgfx_instantiateSolidUI((bsgfx_UISolid) {
        .position = border_position,
        .size = border_size,
        .material_id = $border_outline_color()->id,
        .subtype = subtypes->ui_solid,
    }, element);
    position.z++;
    
   /**
    Background
    */
    bs_vec3 background_position = BS_V3(border_position.x + border_outline_size, border_position.y + border_outline_size, position.z);
    bs_vec2 background_size = BS_V2(border_size.x - border_outline_size * 2, border_size.y - border_outline_size * 2);

    bsgfx_instantiateSolidUI((bsgfx_UISolid) {
        .position = background_position,
        .size = background_size,
        .material_id = $background_color()->id,
        .subtype = subtypes->ui_solid,
    }, element);
    position.z++;
}
