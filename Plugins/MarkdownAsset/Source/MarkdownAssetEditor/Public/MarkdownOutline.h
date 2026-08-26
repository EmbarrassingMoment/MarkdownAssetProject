// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"

/**
 * A single heading extracted from Markdown source, used to populate the
 * outline panel and to address the matching anchor in the HTML preview.
 */
struct FMarkdownHeading
{
	/** Heading text with the leading/trailing ATX markers stripped. */
	FString Text;

	/** Heading level, 1 (H1) through 6 (H6). */
	int32 Level = 1;

	/** Zero-based line index of the heading in the Markdown source. */
	int32 LineIndex = 0;

	/** Zero-based order of appearance; matches the id injected by InjectHeadingAnchors. */
	int32 HeadingIndex = 0;
};

namespace MarkdownOutline
{
	/**
	 * Extracts ATX (#..######) and setext (=== / ---) headings from Markdown source,
	 * skipping fenced and indented code blocks. Heading order matches the order of
	 * <h1>..<h6> tags in md4c's HTML output so indices can address preview anchors.
	 */
	TArray<FMarkdownHeading> ExtractHeadings(const FString& Markdown);

	/**
	 * Inserts sequential id attributes (id="md-h-0", "md-h-1", ...) into every bare
	 * <h1>..<h6> opening tag of the given HTML, enabling scrollIntoView navigation.
	 */
	FString InjectHeadingAnchors(const FString& Html);
}
