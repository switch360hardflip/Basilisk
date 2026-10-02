
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

#define INPUT_FIELD_HEIGHT 20
#define BORDER_ROUNDED_EDGES BS_V4(5.0, 5.0, 5.0, 5.0)

typedef struct {
    bs_String* string;
    bs_Range instance_range;
    bool rendered;
    int select_position;
    int select_size;
} InputField;

typedef struct {
    float timer;
    bsgfx_Material* material;
    bs_Range instance_range;
} InputCaret;

typedef enum {
    INPUT_FIELD_PROJECT_NAME,
    INPUT_FIELD_PROJECT_PATH,

    INPUT_FIELD_COUNT,
} InputFieldId;

static CommonSubtypes _new_project_subtypes_;
static TitleBarButton _new_project_title_bar_buttons_[TITLE_BAR_BUTTON_COUNT];
static InputField _new_project_input_fields_[INPUT_FIELD_COUNT];
static InputFieldId _new_project_selected_input_ = -1;
static InputCaret _new_project_caret_;



  /*==============================================================================
   * Rendering
   *============================================================================*/

void renderTitleBarSubtypes(bs_RendererScope* scope, const CommonSubtypes* subtypes) {
    bs_Queue* queue = scope->queue;
    renderUISolid(scope, queue, subtypes->ui_solid);
    renderUI(scope, queue, subtypes->ui);
    renderFontSubtype(scope, queue, subtypes->text, 0, $fs_bsgfx_font_small());
}

static void renderCreateMenu(bs_RendererScope* scope) {
    bs_Queue* queue = scope->queue;

#ifndef NDEBUG
    bs_beginCommentN(queue, BS_CONSTANT_STRING("Create Menu"));
#endif

    bs_vec4 clear_color = bs_rgbUCharToV4(BS_RGBA(83, 83, 83, 255));
    if (bs_instance()->physical_device->flags & BS_PHYSICAL_DEVICE_SRGB_FORMAT)
        clear_color.xyz = bs_sRGBToLinearV3(&clear_color.xyz);

    bs_clearColor(queue, 0, bs_resolution(bs_scope()->context), &clear_color);
    
    renderTitleBarSubtypes(scope, &_new_project_subtypes_);

#ifndef NDEBUG
    bs_endComment(queue);
#endif
}

static void createMenuPipeline(bs_Context* context) {
    bs_ivec2 resolution = bs_resolution(context);
    bs_Output outputs[] = {
        {
            .subpass = 0,
            .image = context->swapchain_image->image,
            .load_op = BS_ATTACHMENT_LOAD_OP_CLEAR,
            .store_op = BS_ATTACHMENT_STORE_OP_STORE,
            .old_layout = BS_IMAGE_LAYOUT_UNDEFINED,
            .new_layout = BS_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        }
    };

    bs_Renderer renderer = {
        .head = {
            .type = BS_OBJECT_RENDERER,
        },
        .outputs = {.data = outputs, .count = sizeof(outputs) / sizeof(*outputs), .unit_size = sizeof(bs_Output)},
        .dim = resolution,
    };

    bs_Queue* queue = bs_fetch(BSGFX_QUEUES, BSGFX_QUEUE_GRAPHICS)->queue;

    bs_SubpassFunction funcs[] = {
        renderCreateMenu,
    };

    pipeline(queue, &renderer, funcs, sizeof(funcs) / sizeof(*funcs));
}



  /*==============================================================================
   * Logic
   *============================================================================*/

BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, icon)
BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, close_caption)
BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, maximize_caption)
BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, minimize_caption)

BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, title_bar_close)
BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, title_bar_minimize)
BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, title_bar_maximize)

BSGFX_CACHE_COLOR_MATERIAL(title_bar_background, BS_RGBA(83, 83, 83, 255))
BSGFX_CACHE_COLOR_MATERIAL(close_button_background_color, BS_RGBA(226, 42, 39, 255))
BSGFX_CACHE_COLOR_MATERIAL(default_button_background_color, BS_RGBA(93, 93, 93, 255))
BSGFX_CACHE_COLOR_MATERIAL(transparent_color, BS_RGBA(0, 0, 0, 0))
BSGFX_CACHE_COLOR_MATERIAL(input_background_outline_color, BS_RGBA(30, 30, 30, 255))
BSGFX_CACHE_COLOR_MATERIAL(input_background_color, BS_RGBA(42, 42, 42, 255))
BSGFX_CACHE_COLOR_MATERIAL(input_selected_outline_color, BS_RGBA(0, 120, 215, 255))

static void instantiateNewProjectMenuOptions(bs_Context* context, InputField* field) {
    bsgfx_UIInput input = {
        .type = BSGFX_INPUT_STRING,
        .material_id = $magenta_material()->id,
        .placeholder_text_material_id = $blue_material()->id,
        .dimensions = { 128, 32 },
        .select_position = &field->select_position,
        .select_size = &field->select_size,
        .font = _fonts_.selawik,
        .as_string = &field->string,
        .text_subtype = _new_project_subtypes_.text,
    };

    const char* alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789å";
    bs_vec2 width = { 0 , 0 };
    bsgfx_instanceUIInput(&input, BS_V3(0, 0, 0), &width, alphabet);
}



  /*==============================================================================
   * Load
   *============================================================================*/

static bs_CursorIcon onNewProjectSetCursor(bs_Context* context) {
    for (int i = 0; i < INPUT_FIELD_COUNT; i++) {
        if (!_new_project_input_fields_[i].rendered)
            continue;

        bool hovering = bsgfx_hoveringQuadInstance(context, _new_project_subtypes_.ui_solid, _new_project_input_fields_[i].instance_range.offset);

        if (hovering)
            return BS_CURSOR_TEXT;
    }

    return BS_CURSOR_DEFAULT;
}

static void resetNewProjectMenuInputs() {
    _new_project_caret_.timer = 1.0;
    _new_project_selected_input_ = -1;
    _new_project_input_fields_[INPUT_FIELD_PROJECT_NAME].string = bs_stringN(_new_project_input_fields_[INPUT_FIELD_PROJECT_NAME].string, BS_CONSTANT_STRING("My Project"));
    _new_project_caret_.material = $white_material();
}

static void updateInputCaretInstance(InputCaret* caret) {
    bsgfx_InstanceHeader* header = bsgfx_deviceInstanceHeader(_new_project_subtypes_.ui_solid, caret->instance_range.offset);

    bsgfx_Material* white_material = $white_material();
    bsgfx_Material* transparent_material = $transparent_color();

    if (caret->material == transparent_material)
        caret->material = white_material;
    else
        caret->material = transparent_material;

    header->material = caret->material->id;
}

static void tickInputCaret(InputCaret* caret) {
    caret->timer += bs_deltaTime();
    if (caret->timer > 0.5) {
        caret->timer = 0.0;

        updateInputCaretInstance(caret);
        updateInstances();
    }
}

static void onNewProjectMenuShow(bs_Context* context, bool shown) {
    resetNewProjectMenuInputs();
    updateInputCaretInstance(&_new_project_caret_);
    updateInstances();
    //_new_project_input_fields_[INPUT_FIELD_PROJECT_PATH].string = bs_string(_new_project_input_fields_[INPUT_FIELD_PROJECT_PATH].string, BS_CONSTANT_STRING("My Project"));
}

static void onNewProjectMenuMotion(bs_Context* context, int x, int y) {

}

static void instantiateNewProjectMenuOptions(bs_Context* context) {
    static int select_position;
    static int select_size;
    static bs_String* string;

    if (!string)
        string = bs_stringF(NULL, "test 123\nabc wt");

    bsgfx_UIInput input = {
        .type = BSGFX_INPUT_STRING,
        .material_id = $magenta_material()->id,
        .placeholder_text_material_id = $blue_material()->id,
        .dimensions = { 128, 32 },
        .select_position = &select_position,
        .select_size = &select_size,
        .font = _fonts_.selawik,
        .as_string = &string,
        .text_subtype = _new_project_subtypes_.text,
    };

    const char* alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789å";
    bs_vec2 width = { 0 , 0 };
    bsgfx_instanceUIInput(&input, BS_V3(0, 0, 0), &width, alphabet);

    static bs_String* formatted_string;
    formatted_string = bs_string(formatted_string, "");
    int selection_start = select_position;
    int selection_end = select_position + select_size;
    if (selection_start > selection_end) {
        int tmp = selection_start;
        selection_start = selection_end;
        selection_end = tmp;
    }
    for (int i = 0; i < string->len; i++) {
        if (i == select_position)
            formatted_string = bs_appendString(formatted_string, "\033[36m|" "\033[0m");

        if (i == string->len)
            break;

        if (i >= selection_start && i < selection_end) 
            formatted_string = bs_appendString(formatted_string, "\033[46m");
        else 
            formatted_string = bs_appendString(formatted_string, "\033[0m");

        formatted_string = bs_appendChar(formatted_string, string->value[i]);
    }
}

static void onNewProjectMenuInput(bs_Context* context, bs_ContextInputParams params) {
   /**
    Inputs
    */
    if (params.code == BS_LEFT_MOUSE_BUTTON && params.state == BS_INPUT_PRESSED) {
        for (int i = 0; i < INPUT_FIELD_COUNT; i++) {
            if (!_new_project_input_fields_[i].rendered)
                continue;

            bool hovering = bsgfx_hoveringQuadInstance(context, _new_project_subtypes_.ui_solid, _new_project_input_fields_[i].instance_range.offset);

            if (hovering) {
                resetNewProjectMenuInputs();
                updateInputCaretInstance(&_new_project_caret_);
                updateInstances();
                _new_project_selected_input_ = i;
                return;
            }
        }
    }

    resetNewProjectMenuInputs();
    updateInputCaretInstance(&_new_project_caret_);
}

static void onNewProjectMenuResize(bs_Context* context, int w, int h) {
    tickInputCaret(&_new_project_caret_);
}

void onCreateMenuTick(bs_Context* context, void* params);
static void createNewProjectMenuWindow(bs_Context* context) {
    const bs_ivec2 dimensions = { 500, 400 };
    bs_window(context, basilisk.context, (bs_ContextListener) {
        .tick = onCreateMenuTick,
        .set_cursor = onNewProjectSetCursor,
        .show = onNewProjectMenuShow,
        .motion = onNewProjectMenuMotion,
        .input = onNewProjectMenuInput,
        .resize = onNewProjectMenuResize,
    }, dimensions.x, dimensions.y, "New Project", BS_WINDOW_NO_TITLE_BAR);
    bs_addBorderPadding(context, BORDER_PADDING);
}

void onCreateMenuTick(bs_Context* context, void* params) {
    if (context->hidden)
        return;

    if (_new_project_selected_input_ >= 0)
        tickInputCaret(&_new_project_caret_);

    if (bs_scope()->resizing)
        updateInstances();

    updateInstances();
    //bool title_bar_has_changes = onTitleBarTick();
    bsgfx_computeContextCamera();
    createMenuPipeline(context);

    titleBarButtonTest(&_new_project_subtypes_, _new_project_title_bar_buttons_, TITLE_BAR_BUTTON_CLOSE, $close_button_background_color());

    if (_new_project_selected_input_ >= 0) {
        instantiateNewProjectMenuOptions(context, _new_project_input_fields_+ _new_project_selected_input_);
    }


    if (bs_inputUpOnce(BS_LEFT_MOUSE_BUTTON)) {
        if (_new_project_title_bar_buttons_[TITLE_BAR_BUTTON_CLOSE].hovering) {
            bs_hideWindow(context);
            bs_destroyContext(context);
            createNewProjectMenuWindow(context);
            updateInstances();
        }
    }
}

static void centerNewProjectMenu() {
    const bs_ivec2 dimensions = { 500, 400 };

    bs_Context* main_context = bs_fetch(BSGFX_CONTEXTS, BSGFX_CONTEXT_MAIN)->context;
    bs_ivec2 position = bs_windowPosition(main_context);
    bs_Object* context_object = bs_fetch(BASILISK_CONTEXTS, BASILISK_CONTEXT_NEW_PROJECT);

    bs_moveWindow(
        context_object->context, 
        position.x + (main_context->dimensions.x - dimensions.x) / 2.0, 
        position.y - (main_context->dimensions.y + dimensions.y) / 2.0
    );
}

void iniNewProjectMenu() {
    bs_Context* main_context = bs_fetch(BSGFX_CONTEXTS, BSGFX_CONTEXT_MAIN)->context;

    const bs_ivec2 dimensions = { 500, 400 };

    bs_Object* context_object = BS_CONTEXT(BASILISK_CONTEXTS, BASILISK_CONTEXT_NEW_PROJECT, BS_OBJECT_IN_FLIGHT_BIT);
    createNewProjectMenuWindow(context_object->context);
    resetNewProjectMenuInputs();
}

void onCreateProjectFromContextMenu() {
    bs_Object* context_object = bs_fetch(BASILISK_CONTEXTS, BASILISK_CONTEXT_NEW_PROJECT);
    centerNewProjectMenu();
    bs_showWindow(context_object->context);
}



  /*==============================================================================
   * Instantiating
   *============================================================================*/

static void instantiateInputField(bs_Context* context, InputField* field, const char* name, bs_vec3 position, float width, int padding) {
    bsgfx_UIElement element;

    InputFieldId id = field - _new_project_input_fields_;

    const int selected_size = 3;
    const int outline_size = 1;
    const int input_indent = 4;

    bs_ivec2 resolution = bs_resolution(context);

    field->rendered = true;

   /**
    Left aligned
    */
    position.x += padding;

    bsgfx_UIText text_ui = {
        .position = position,
        .font = _fonts_.selawik,
        .as_ascii = name,
        .px_size = 13,
        .align = { 0, BASILISK_TITLE_BAR_HEIGHT },
        .subtype = _new_project_subtypes_.text,
        .align.y = INPUT_FIELD_HEIGHT + outline_size * 2,
    };

    bsgfx_instantiateTextUI(text_ui, &element);

   /**
    Right aligned
    */
    position.x += resolution.x;
    position.x -= width;
    position.x -= padding * 2;

    if (id == _new_project_selected_input_) {
        bsgfx_instantiateSolidUI((bsgfx_UISolid) {
            .position = { position.x - selected_size, position.y - selected_size, position.z },
            .size = { width + selected_size * 2, INPUT_FIELD_HEIGHT + selected_size * 2 },
            .material_id = $input_selected_outline_color()->id,
            .subtype = _new_project_subtypes_.ui_solid,
            .borders = BORDER_ROUNDED_EDGES,
        }, &element);
        position.z++;
    }

    field->instance_range = bsgfx_instantiateSolidUI((bsgfx_UISolid) {
        .position = { position.x - outline_size, position.y - outline_size, position.z },
        .size = { width + outline_size * 2, INPUT_FIELD_HEIGHT + outline_size * 2 },
        .material_id = $input_background_outline_color()->id,
        .subtype = _new_project_subtypes_.ui_solid,
        .borders = BORDER_ROUNDED_EDGES,
    }, &element);
    position.z++;

    bsgfx_instantiateSolidUI((bsgfx_UISolid) {
        .position = position,
        .size = { width, INPUT_FIELD_HEIGHT },
        .material_id = $input_background_color()->id,
        .subtype = _new_project_subtypes_.ui_solid,
        .borders = BORDER_ROUNDED_EDGES,
    }, &element);
    position.z++;

    if (field->string) {
        position.x += input_indent;
        text_ui = (bsgfx_UIText){
            .position = position,
            .font = _fonts_.selawik,
            .as_ascii = field->string->value,
            .px_size = 13,
            .align = { 0, BASILISK_TITLE_BAR_HEIGHT },
            .subtype = _new_project_subtypes_.text,
            .align.y = INPUT_FIELD_HEIGHT + outline_size * 2,
        };

        bsgfx_instantiateTextUI(text_ui, &element);
        position.x -= input_indent;
    }

    if (id == _new_project_selected_input_) {
        bsgfx_UIElement element;
        position.x += field->select_position * 16;
        _new_project_caret_.instance_range = bsgfx_instantiateSolidUI((bsgfx_UISolid) {
            .position = position,
            .size = { 2, INPUT_FIELD_HEIGHT },
            .material_id = _new_project_caret_.material->id,
            .subtype = _new_project_subtypes_.ui_solid,
        }, &element);

        position.z++;
    }
}

static void instantiateNewProjectMenuContents(bs_Context* context) {
    const int padding = 16;
    const int element_padding = 8;
    const int input_width = 350;

    bs_ivec2 resolution = bs_resolution(context);
    bs_vec3 position = { 0.0, resolution.y - BASILISK_TITLE_BAR_HEIGHT, 0.0 };

    position.x += context->border_padding;
  //  position.x += padding;
    position.y -= padding;

    position.y -= INPUT_FIELD_HEIGHT;
    instantiateInputField(context, _new_project_input_fields_ + INPUT_FIELD_PROJECT_NAME, "Project Name", position, input_width, padding);
    position.y -= element_padding;

    position.y -= INPUT_FIELD_HEIGHT;
    instantiateInputField(context, _new_project_input_fields_ + INPUT_FIELD_PROJECT_PATH, "Project Path", position, input_width, padding);
    position.y -= element_padding;
}

void instantiateNewProjectMenu(bs_Context* context) {
    bsgfx_InstanceType* quad_instance_type = bsgfx_instanceTypes()[BSGFX_INSTANCE_TYPE_2_QUAD];
    bs_Range range = { 0, 6 }; // TODO: remove manual range
    bs_Batch* batch = bs_fetch(BSGFX_BATCHES, BSGFX_BATCH_QUAD_INSTANCED)->batch;

    if (!_new_project_subtypes_.text)
        bsgfx_subtype(quad_instance_type, batch, 0, range, &_new_project_subtypes_.text);
    if (!_new_project_subtypes_.ui)
        bsgfx_subtype(quad_instance_type, batch, 0, range, &_new_project_subtypes_.ui);
    if (!_new_project_subtypes_.ui_solid)
        bsgfx_subtype(quad_instance_type, batch, 0, range, &_new_project_subtypes_.ui_solid);

    instantiateBaseUI(context, &_new_project_subtypes_);
    instantiateNewProjectMenuContents(context);

    CommonSubtypes* subtypes = &_new_project_subtypes_;
    TitleBarButton* buttons = _new_project_title_bar_buttons_;
    for (int i = 0; i < TITLE_BAR_BUTTON_COUNT; i++) {
        buttons[i].hovering = buttons[i].hover_once = buttons[i].hover_release = false;
    }

    bs_ivec2 resolution = bs_resolution(context);
    bs_vec2 title_bar_size = { resolution.x, BASILISK_TITLE_BAR_HEIGHT };

    bsgfx_AtlasCache* icon_atlas_cache = $BSMOD_ATLAS_UI_icon();
    bsgfx_AtlasCache* close_caption = $BSMOD_ATLAS_UI_title_bar_close();
    bsgfx_AtlasCache* maximize_caption = $BSMOD_ATLAS_UI_title_bar_maximize();
    bsgfx_AtlasCache* minimize_caption = $BSMOD_ATLAS_UI_title_bar_minimize();

    bsgfx_Material* transparent_material = $transparent_color();

    bs_vec3 position;
    bsgfx_UIElement element_v;
    bsgfx_UIElement* element = &element_v;
    bool hovering;

    // title icon
    //position = bsgfx_seekTopLeftUI(title_bar_size);
    position = BS_V3(0, resolution.y - title_bar_size.y, 0);
    /**
     Background
     */
     /*
     bsgfx_instantiateSolidUI((bsgfx_UISolid) {
         .position = position,
         .size = title_bar_size,
         .material_id = $title_bar_background()->id,
         .subtype = bsgfx_subtypes()[BSGFX_SUBTYPE_UI_COLOR],
     }, element);
     position.z++;
     */

     /**
      Icon
      */
    bsgfx_instantiateAtlasIconUI((bsgfx_UIIcon) {
        .position = position,
        .cache = icon_atlas_cache,
        .subtype = subtypes->ui,
        .align = { 32, BASILISK_TITLE_BAR_HEIGHT },
    }, element);

    /**
     File button
     */
    position.x += element->size.x;
    position.x += 16.0;

    instantiateTitleBarTextUI(subtypes, "New Project", transparent_material, position, title_bar_size);

    //bs_Context* ctx = contextFromMenuType(CONTEXT_MENU_FILE);

    //if (bs_inputDownOnce(BS_LEFT_MOUSE_BUTTON) || bs_inputDownOnce(BS_LEFT_MOUSE_BUTTON_NON_CLIENT) || 
    //    bs_inputDownOnce(BS_RIGHT_MOUSE_BUTTON) || bs_inputDownOnce(BS_RIGHT_MOUSE_BUTTON_NON_CLIENT))
    //{
    //    if (hovering)
    //        toggleContextMenu(BS_IV2(position.x + TITLE_BAR_BUTTON_PADDING_X / 2, position.y - TITLE_BAR_BUTTON_PADDING_Y * 2), CONTEXT_MENU_FILE);
    //}

    position.x = title_bar_size.x;

    const int close_button_width = 48;

    position.x -= close_button_width;
    instantiateTitleBarButtonUI(subtypes, buttons, TITLE_BAR_BUTTON_CLOSE, close_caption, transparent_material, position, title_bar_size, close_button_width);

}
