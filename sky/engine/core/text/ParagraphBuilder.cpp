// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sky/engine/core/text/ParagraphBuilder.h"

#include "sky/engine/core/rendering/RenderInline.h"
#include "sky/engine/core/rendering/RenderParagraph.h"
#include "sky/engine/core/rendering/RenderText.h"
#include "sky/engine/core/rendering/style/RenderStyle.h"
#include "sky/engine/platform/text/LocaleToScriptMapping.h"
#include "sky/engine/tonic/dart_args.h"
#include "sky/engine/tonic/dart_binding_macros.h"
#include "sky/engine/tonic/dart_converter.h"
#include "sky/engine/tonic/dart_library_natives.h"

namespace blink {
namespace {

RenderParagraph* createRenderParagraph(RenderStyle* parentStyle)
{
    RefPtr<RenderStyle> style = RenderStyle::create();
    style->inheritFrom(parentStyle);
    style->setDisplay(PARAGRAPH);

    RenderParagraph* renderParagraph = new RenderParagraph();
    renderParagraph->setStyle(style.release());
    return renderParagraph;
}

float getComputedSizeFromSpecifiedSize(float specifiedSize)
{
    if (specifiedSize < std::numeric_limits<float>::epsilon())
        return 0.0f;
    return specifiedSize;
}

void createFontForDocument(RenderStyle* style)
{
    FontDescription fontDescription = FontDescription();
    fontDescription.setScript(localeToScriptCodeForFontSelection(style->locale()));

    // Using 14px default to match Material Design English Body1:
    // http://www.google.com/design/spec/style/typography.html#typography-typeface
    const float defaultFontSize = 14.0;

    fontDescription.setSpecifiedSize(defaultFontSize);
    fontDescription.setComputedSize(defaultFontSize);

    FontOrientation fontOrientation = Horizontal;
    NonCJKGlyphOrientation glyphOrientation = NonCJKGlyphOrientationVerticalRight;

    fontDescription.setOrientation(fontOrientation);
    fontDescription.setNonCJKGlyphOrientation(glyphOrientation);
    style->setFontDescription(fontDescription);
    style->font().update(nullptr);
}

Color getColorFromARGB(int argb) {
  return Color(
    (argb & 0x00FF0000) >> 16,
    (argb & 0x0000FF00) >> 8,
    (argb & 0x000000FF) >> 0,
    (argb & 0xFF000000) >> 24
  );
}

// TextStyle

const int tsColorIndex = 1;
const int tsTextDecorationIndex = 2;
const int tsTextDecorationColorIndex = 3;
const int tsTextDecorationStyleIndex = 4;
const int tsFontWeightIndex = 5;
const int tsFontStyleIndex = 6;
const int tsFontFamilyIndex = 7;
const int tsFontSizeIndex = 8;
const int tsLetterSpacingIndex = 9;
const int tsWordSpacingIndex = 10;
const int tsLineHeightIndex = 11;

const int tsColorMask = 1 << tsColorIndex;
const int tsTextDecorationMask = 1 << tsTextDecorationIndex;
const int tsTextDecorationColorMask = 1 << tsTextDecorationColorIndex;
const int tsTextDecorationStyleMask = 1 << tsTextDecorationStyleIndex;
const int tsFontWeightMask = 1 << tsFontWeightIndex;
const int tsFontStyleMask = 1 << tsFontStyleIndex;
const int tsFontFamilyMask = 1 << tsFontFamilyIndex;
const int tsFontSizeMask = 1 << tsFontSizeIndex;
const int tsLetterSpacingMask = 1 << tsLetterSpacingIndex;
const int tsWordSpacingMask = 1 << tsWordSpacingIndex;
const int tsLineHeightMask = 1 << tsLineHeightIndex;

// ParagraphStyle

const int psTextAlignIndex = 1;
const int psTextBaselineIndex = 2;
const int psLineHeightIndex = 3;

const int psTextAlignMask = 1 << psTextAlignIndex;
const int psTextBaselineMask = 1 << psTextBaselineIndex;
const int psLineHeightMask = 1 << psLineHeightIndex;

}  // namespace

static void ParagraphBuilder_constructor(Dart_NativeArguments args) {
  DartCallConstructor(&ParagraphBuilder::create, args);
}

IMPLEMENT_WRAPPERTYPEINFO(ui, ParagraphBuilder);

#define FOR_EACH_BINDING(V) \
  V(ParagraphBuilder, pushStyle) \
  V(ParagraphBuilder, pop) \
  V(ParagraphBuilder, addText) \
  V(ParagraphBuilder, build)

FOR_EACH_BINDING(DART_NATIVE_CALLBACK)

void ParagraphBuilder::RegisterNatives(DartLibraryNatives* natives) {
  natives->Register({
    { "ParagraphBuilder_constructor", ParagraphBuilder_constructor, 1, true },
FOR_EACH_BINDING(DART_REGISTER_NATIVE)
  });
}

ParagraphBuilder::ParagraphBuilder()
{
    createRenderView();
    m_renderParagraph = createRenderParagraph(m_renderView->style());
    m_currentRenderObject = m_renderParagraph;
    m_renderView->addChild(m_currentRenderObject);
}

ParagraphBuilder::~ParagraphBuilder()
{
}

void ParagraphBuilder::pushStyle(Int32List& encoded, const std::string& fontFamily, double fontSize, double letterSpacing, double wordSpacing, double lineHeight)
{
    DCHECK(encoded.num_elements() == 7);
    RefPtr<RenderStyle> style = RenderStyle::create();
    style->inheritFrom(m_currentRenderObject->style());

    int32_t mask = encoded[0];

    if (mask & tsColorMask)
      style->setColor(getColorFromARGB(encoded[tsColorIndex]));

    if (mask & tsTextDecorationMask) {
      style->setTextDecoration(static_cast<TextDecoration>(encoded[tsTextDecorationIndex]));
      style->applyTextDecorations();
    }

    if (mask & tsTextDecorationColorMask)
      style->setTextDecorationColor(StyleColor(getColorFromARGB(encoded[tsTextDecorationColorIndex])));

    if (mask & tsTextDecorationStyleMask)
      style->setTextDecorationStyle(static_cast<TextDecorationStyle>(encoded[tsTextDecorationStyleIndex]));

    if (mask & (tsFontWeightMask | tsFontStyleMask | tsFontFamilyMask | tsFontSizeMask | tsLetterSpacingMask | tsWordSpacingMask)) {
      FontDescription fontDescription = style->fontDescription();

      if (mask & tsFontWeightMask)
        fontDescription.setWeight(static_cast<FontWeight>(encoded[tsFontWeightIndex]));

      if (mask & tsFontStyleMask)
        fontDescription.setStyle(static_cast<FontStyle>(encoded[tsFontStyleIndex]));

      if (mask & tsFontFamilyMask) {
        FontFamily family;
        family.setFamily(String::fromUTF8(fontFamily));
        fontDescription.setFamily(family);
      }

      if (mask & tsFontSizeMask) {
        fontDescription.setSpecifiedSize(fontSize);
        fontDescription.setIsAbsoluteSize(true);
        fontDescription.setComputedSize(getComputedSizeFromSpecifiedSize(fontSize));
      }

      if (mask & tsLetterSpacingMask)
        fontDescription.setLetterSpacing(letterSpacing);

      if (mask & tsWordSpacingMask)
        fontDescription.setWordSpacing(wordSpacing);

      style->setFontDescription(fontDescription);
      style->font().update(nullptr);
    }

    if (mask & tsLineHeightMask) {
      style->setLineHeight(Length(lineHeight * 100.0, Percent));
    }

    encoded.Release();

    RenderObject* span = new RenderInline();
    span->setStyle(style.release());
    m_currentRenderObject->addChild(span);
    m_currentRenderObject = span;
}

void ParagraphBuilder::pop()
{
    if (m_currentRenderObject)
        m_currentRenderObject = m_currentRenderObject->parent();
}

void ParagraphBuilder::addText(const std::string& text)
{
    if (!m_currentRenderObject)
        return;
    RenderText* renderText = new RenderText(String::fromUTF8(text).impl());
    RefPtr<RenderStyle> style = RenderStyle::create();
    style->inheritFrom(m_currentRenderObject->style());
    renderText->setStyle(style.release());
    m_currentRenderObject->addChild(renderText);
}

PassRefPtr<Paragraph> ParagraphBuilder::build(Int32List& encoded, double lineHeight)
{
    DCHECK(encoded.num_elements() == 3);
    int32_t mask = encoded[0];

    if (mask) {
      RefPtr<RenderStyle> style = RenderStyle::clone(m_renderParagraph->style());

      if (mask & psTextAlignMask)
        style->setTextAlign(static_cast<ETextAlign>(encoded[psTextAlignIndex]));

      if (mask & psTextBaselineMask) {
        // TODO(abarth): Implement TextBaseline. The CSS version of this
        // property wasn't wired up either.
      }

      if (mask & psLineHeightMask)
        style->setLineHeight(Length(lineHeight * 100.0, Percent));

      m_renderParagraph->setStyle(style.release());
    }

    encoded.Release();

    m_currentRenderObject = nullptr;
    return Paragraph::create(m_renderView.release());
}

void ParagraphBuilder::createRenderView()
{
    RefPtr<RenderStyle> style = RenderStyle::create();
    style->setRTLOrdering(LogicalOrder);
    style->setZIndex(0);
    style->setUserModify(READ_ONLY);
    createFontForDocument(style.get());

    m_renderView = adoptPtr(new RenderView());
    m_renderView->setStyle(style.release());
}

} // namespace blink
