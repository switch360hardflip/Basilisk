
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
#include <pipeline.h>
#include <basilisk.h>

/**
 Low Resolution Subpass 0
 Renders pixelated geometry
 Writes to
   BSGFX_IMAGE_LO_RES_DEPTH
   BSGFX_IMAGE_LO_RES_COLOR
   BSGFX_IMAGE_LO_RES_NORMAL
   BSGFX_IMAGE_LO_RES_POSITION
   BSGFX_IMAGE_LO_RES_INDEX
 */
static void _bsgfx_loResSubpass0() {
}

 /**
  High Resolution Subpass 0
  Writes to the swapchain
  */

void pipeline(bs_Queue* queue, bs_Renderer* renderer, bs_SubpassFunction callbacks[], int callbacks_count) {
  //  if (bs_scope()->context->swapchain_ok) {
        bs_acquire();
  //  }
  //  if (!bs_scope()->context->swapchain_ok) {
  //      return;
  //  }

    if (bs_resetQueue(queue) == BS_RESULT_OK) {
        if (renderer->render_pass) {
            bs_runPass(queue, renderer, callbacks, callbacks_count);
        }
        else {
            bs_Output* output = bs_fetchUnit(&renderer->outputs, 0);
            bs_transition(queue, output->image, 0, BS_IMAGE_LAYOUT_UNDEFINED, BS_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            bs_RendererScope scope = bs_beginRender(queue, renderer);

            for (int i = 0; i < callbacks_count; i++)
                callbacks[i](&scope);

            bs_endRender(queue, renderer);
            bs_transition(queue, output->image, 0, BS_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, BS_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        }

        bs_WaitSemaphore wait_semaphores[] = {
            bs_acquisitionSemaphore(),
        };
        int wait_semaphores_count = sizeof(wait_semaphores) / sizeof(*wait_semaphores);

        bs_pushQueue(queue, wait_semaphores_count, wait_semaphores);
    }

    bs_Queue* wait_queues[] = {
        queue
    };

    bs_present(queue, wait_queues, sizeof(wait_queues) / sizeof(*wait_queues));
    bs_awaitQueue(queue);
}

bs_Object* createHiResRenderer(bs_Context* context, int id) {
    bs_Object* hi_res = BS_RENDERER(BASILISK_RENDERERS, id, BS_OBJECT_SWAPCHAIN_IMAGE_BIT);
    if (bs_renderer(hi_res, 0) == BS_RESULT_OK) {
        bs_autoResizeRenderer(hi_res->renderer, context);

        bs_ivec2 resolution = bs_resolution(context);
        bs_Object* hi_res_0_depth = BS_IMAGE(-1, -1, 0);
        if (bs_image(hi_res_0_depth, resolution, 0, BS_FORMAT_D32_SFLOAT_S8_UINT, BS_IMAGE_ATTACHMENT_BIT) == BS_RESULT_OK) {
            bs_output(hi_res->renderer, (bs_Output) {
                .subpass = 0,
                .image = hi_res_0_depth->image,
                .load_op = BS_ATTACHMENT_LOAD_OP_CLEAR,
                .store_op = BS_ATTACHMENT_STORE_OP_STORE,
                .old_layout = BS_IMAGE_LAYOUT_UNDEFINED,
                .new_layout = BS_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            });

            bs_output(hi_res->renderer, (bs_Output) {
                .subpass = 0,
                .image = context->swapchain_image->image,
                .load_op = BS_ATTACHMENT_LOAD_OP_CLEAR,
                .store_op = BS_ATTACHMENT_STORE_OP_STORE,
                .old_layout = BS_IMAGE_LAYOUT_UNDEFINED,
                .new_layout = BS_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            });
            /** dependencies */
            bs_dependency(hi_res->renderer, -1, 0, BS_DEPENDENCY_BY_REGION_BIT,
                BS_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                BS_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | BS_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                BS_ACCESS_MEMORY_READ_BIT,
                BS_ACCESS_COLOR_ATTACHMENT_READ_BIT | BS_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | BS_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
            hi_res->renderer->_->framebuffer;
            bs_renderPass(hi_res->renderer);
            bs_framebuffer(hi_res->renderer, bs_resolution(context));
        }
    }

    return hi_res;
}

void createRenderers() {
    createHiResRenderer(bs_fetch(BSGFX_CONTEXTS, BSGFX_CONTEXT_MAIN)->context, BASILISK_RENDERER_MAIN);
    if (bs_exists(BASILISK_CONTEXTS, BASILISK_CONTEXT_TITLE_BAR))
        createHiResRenderer(bs_fetch(BASILISK_CONTEXTS, BASILISK_CONTEXT_TITLE_BAR)->context, BASILISK_RENDERER_TITLE_BAR);
}
