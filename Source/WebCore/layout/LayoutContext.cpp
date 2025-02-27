/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "LayoutContext.h"

#if ENABLE(LAYOUT_FORMATTING_CONTEXT)

#include "BlockFormattingContext.h"
#include "BlockFormattingState.h"
#include "DisplayBox.h"
#include "DisplayPainter.h"
#include "InlineFormattingContext.h"
#include "InlineFormattingState.h"
#include "InvalidationContext.h"
#include "InvalidationState.h"
#include "LayoutBox.h"
#include "LayoutContainer.h"
#include "LayoutPhase.h"
#include "LayoutTreeBuilder.h"
#include "RenderStyleConstants.h"
#include "RenderView.h"
#include "RuntimeEnabledFeatures.h"
#include "TableFormattingContext.h"
#include "TableFormattingState.h"
#include <wtf/IsoMallocInlines.h>

namespace WebCore {
namespace Layout {

WTF_MAKE_ISO_ALLOCATED_IMPL(LayoutContext);

LayoutContext::LayoutContext(LayoutState& layoutState)
    : m_layoutState(layoutState)
{
}

void LayoutContext::layout(const LayoutSize& rootContentBoxSize, InvalidationState& invalidationState)
{
    // Set the geometry on the root.
    // Note that we never layout the root box. It has to have an already computed geometry (in case of ICB, it's the view geometry).
    // ICB establishes the initial BFC, but it does not live in a formatting context and while a non-ICB root(subtree layout) has to have a formatting context,
    // we could not lay it out even if we wanted to since it's outside of this LayoutContext.
    auto& displayBox = layoutState().displayBoxForRootLayoutBox();
    displayBox.setHorizontalMargin({ });
    displayBox.setHorizontalComputedMargin({ });
    displayBox.setVerticalMargin({ });
    displayBox.setBorder({ });
    displayBox.setPadding({ });
    displayBox.setTopLeft({ });
    displayBox.setContentBoxHeight(rootContentBoxSize.height());
    displayBox.setContentBoxWidth(rootContentBoxSize.width());

    layoutWithPreparedRootGeometry(invalidationState);
}

void LayoutContext::layoutWithPreparedRootGeometry(InvalidationState& invalidationState)
{
    PhaseScope scope(Phase::Type::Layout);

    auto& formattingContextRootsForLayout = invalidationState.formattingContextRoots();
    // When invalidation is empty, we assume constraint mutation and start running layout on the context root. Layout logic should be able to figure out the damage.
    if (formattingContextRootsForLayout.computesEmpty())
        return layoutFormattingContextSubtree(m_layoutState.root(), invalidationState);

    for (auto& formattingContextRoot : formattingContextRootsForLayout)
        layoutFormattingContextSubtree(formattingContextRoot, invalidationState);
}

void LayoutContext::layoutFormattingContextSubtree(const Container& formattingContextRoot, InvalidationState& invalidationState)
{
    RELEASE_ASSERT(formattingContextRoot.establishesFormattingContext());
    auto formattingContext = createFormattingContext(formattingContextRoot, layoutState());
    auto& displayBox = layoutState().displayBoxForLayoutBox(formattingContextRoot);

    auto horizontalConstraints = HorizontalConstraints { displayBox.contentBoxLeft(), displayBox.contentBoxWidth() };
    auto verticalConstraints = VerticalConstraints { displayBox.contentBoxTop(), { } };
    formattingContext->layoutInFlowContent(invalidationState, horizontalConstraints, verticalConstraints);

    // FIXME: layoutFormattingContextSubtree() does not perform layout on the root, rather it lays out the root's content.
    // It constructs an FC for descendant boxes and runs layout on them. The formattingContextRoot is laid out in the FC in which it lives (parent formatting context).
    // It also means that the formattingContextRoot has to have a valid/clean geometry at this point.
    {
        auto horizontalConstraints = HorizontalConstraints { displayBox.paddingBoxLeft(), displayBox.paddingBoxWidth() };
        auto verticalConstraints = VerticalConstraints { displayBox.paddingBoxTop(), displayBox.paddingBoxHeight() };
        formattingContext->layoutOutOfFlowContent(invalidationState, horizontalConstraints, verticalConstraints);
    }
}

std::unique_ptr<FormattingContext> LayoutContext::createFormattingContext(const Container& formattingContextRoot, LayoutState& layoutState)
{
    ASSERT(formattingContextRoot.establishesFormattingContext());
    if (formattingContextRoot.establishesInlineFormattingContext()) {
        auto& inlineFormattingState = layoutState.ensureInlineFormattingState(formattingContextRoot);
        return makeUnique<InlineFormattingContext>(formattingContextRoot, inlineFormattingState);
    }

    if (formattingContextRoot.establishesBlockFormattingContext()) {
        ASSERT(formattingContextRoot.establishesBlockFormattingContextOnly());
        auto& blockFormattingState = layoutState.ensureBlockFormattingState(formattingContextRoot);
        return makeUnique<BlockFormattingContext>(formattingContextRoot, blockFormattingState);
    }

    if (formattingContextRoot.establishesTableFormattingContext()) {
        auto& tableFormattingState = layoutState.ensureTableFormattingState(formattingContextRoot);
        return makeUnique<TableFormattingContext>(formattingContextRoot, tableFormattingState);
    }

    CRASH();
}

void LayoutContext::paint(const LayoutState& layoutState, GraphicsContext& context, const IntRect& dirtyRect)
{
    Display::Painter::paint(layoutState, context, dirtyRect);
}

}
}

#endif
