# Kingdom Key UI artwork

Generated with the built-in image-generation tool on 2 October 2026. The user's Kingdom Key image supplied the final silhouette and proportions; the original 48px Ordon Sword icon supplied shading and outline references. The user approved this fresh version after rejecting two earlier drafts. Original game icon reference files and rejected drafts are not distributed here.

Raster master: `../model/kingdom-key-icon-source.png`.
Prepared transparent 192px game asset: `res/ui/tex1_48x48_7a16cbeebf26f7d6_3b746898124fa62c_9.png`.
The optional Windows authoring helper `tools/prepare_ui_icon.ps1` resizes the master with transparency retained. Normal platform builds use the prepared PNG as-is, with no image processing dependency.

The button icon uses the approved artwork as-is (tip upper left). The standard HUD's native 76-degree sword rotation is temporarily removed for this item so the result matches the approved image; its original angle is restored after drawing. The Collection/pause icon uses the exact same texture with a horizontal UV mirror (tip upper right). Mirroring is scoped to the Collection draw; the original pane UV coordinates are restored immediately afterward. There is no second generated interpretation of the artwork.

The filename identifies the shared original 48px CI8 texture and referenced palette in the Collection and itemicon archives. The higher-resolution replacement does not change native pane dimensions. Touch buttons reuse the PNG through the built-in mod image provider.

## Final generation prompt

Create a fresh transparent Twilight Princess game inventory icon of the exact Kingdom Key shown in reference image 1. Use image 1 as the authoritative weapon design: faithfully trace its silhouette and proportions, especially the exact jagged shape of the silver key teeth at the very end of the shaft. Do not redesign, reinterpret or embellish that shape. Keep the same upper-left blade tip to lower-right handle direction as image 1 so the unusual tooth geometry is preserved accurately. Use the same silver shaft and teeth, gold rectangular guard, black grip, blue collar, chain and mouse-head charm. Reference image 2 is ONLY the Twilight Princess UI ART STYLE: simple hand-painted muted metal shading, warm dark brown edge and a thin tan outer rim. Apply that restrained game icon treatment to the accurate weapon from image 1, with simplified shading for a 48px icon. This is not a shiny studio render, and not heavy pixel art. One complete weapon centered on a square transparent canvas, leave a clear margin around every part including the chain. True transparent background. No text, no symbols except the actual charm, no effects, no framing. Shape accuracy to image 1 matters more than ornament or decorative shading. The blade-end is plain smooth metal with precisely the same cut edge as image 1.

