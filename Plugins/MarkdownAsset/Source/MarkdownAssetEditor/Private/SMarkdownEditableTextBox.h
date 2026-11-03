// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/SMultiLineEditableText.h"

/**
 * SMultiLineEditableTextBox that also exposes the selection and font API of the
 * SMultiLineEditableText it wraps. The box itself only forwards GetSelectedText,
 * GoTo and ScrollTo; Find & Replace needs the selection range and the editor
 * preferences need to change the font after construction.
 *
 * No SLATE_BEGIN_ARGS of its own, so SNew takes the base class's arguments and
 * Construct.
 */
class SMarkdownEditableTextBox : public SMultiLineEditableTextBox
{
public:
	/** The current selection; empty (both ends at the cursor) when nothing is selected. */
	FTextSelection GetSelection() const
	{
		return EditableText->GetSelection();
	}

	/** Selects the range and puts the cursor at InCursorLocation. */
	void SelectText(const FTextLocation& InSelectionStart, const FTextLocation& InCursorLocation)
	{
		EditableText->SelectText(InSelectionStart, InCursorLocation);
	}

	/** Changes the editor font after construction. */
	void SetFont(const TAttribute<FSlateFontInfo>& InNewFont)
	{
		EditableText->SetFont(InNewFont);
	}
};
