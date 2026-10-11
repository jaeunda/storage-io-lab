# SVG design assets

Five vector figures explain the actual implementation and measurements. They share the typography and restrained palette of the profile's compiler, Linux filesystem, and xv6 documentation, with layouts adapted to storage pages, address maps, and reusable slots.

| Figure | Content |
| --- | --- |
| [storage-overview.svg](storage-overview.svg) | The three independent mechanisms and their state responsibilities |
| [flash-update.svg](flash-update.svg) | Eight-page geometry, copy-out/copy-back stages, and full-update driver counts |
| [hybrid-mapping.svg](hybrid-mapping.svg) | Spare LSNs, backward lookup, version selection, and mapping relocation |
| [record-reuse.svg](record-reuse.svg) | Byte offsets, deleted-slot links, insertion, and measured fixed size |
| [measured-results.svg](measured-results.svg) | Hybrid write-amplification bars generated from the saved JSON |

## Design and portability

- Noto Sans Regular and Bold provide modern sans-serif typography. Labels are converted into SVG paths, so GitHub does not need the font installed or an external font request.
- A 1,120-unit viewBox, generous margins, white panels, light slate background, navy type, and thin rules keep the diagrams legible at README width.
- Blue identifies existing/live data, teal identifies current mappings or reuse results, and amber identifies replacement/deletion work. Text labels also express those roles; color is not the only cue.
- Every SVG has a descriptive root `title` and `desc`; outlined label groups retain their strings in `aria-label` and `title`. README images also have descriptive alt text.
- The SVGs contain no raster images, scripts, external resources, or font binaries. PNG previews were used for inspection outside the repository.

The label source and layout are editable in [../../tools/render_diagrams.py](../../tools/render_diagrams.py). [Regeneration instructions](../../tools/README.md). Noto Sans glyph outlines are distributed under the included [SIL Open Font License](NotoSans-OFL.txt).
