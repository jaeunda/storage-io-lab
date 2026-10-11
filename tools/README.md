# Diagram generation

[render_diagrams.py](render_diagrams.py) draws all five README SVGs and converts Noto Sans glyphs into vector outlines. It checks label extents and panel widths, retains accessible label text, and loads chart values from [../experiments/results/latest.json](../experiments/results/latest.json).

## Regenerate

Provide `NotoSans-Regular.ttf` and `NotoSans-Bold.ttf` from the Noto Sans family in one directory. Use the original static fonts from [Noto Sans](https://github.com/notofonts/noto-fonts/tree/main/hinted/ttf/NotoSans), rather than substituting another font; metrics affect layout. Their license is included with the assets.

```bash
python3 -m venv /tmp/storage-diagram-tools
/tmp/storage-diagram-tools/bin/pip install 'fonttools==4.66.1'
/tmp/storage-diagram-tools/bin/python tools/render_diagrams.py /path/to/noto-fonts
```

Run from the repository root. The default output is `docs/assets/`; pass `--output /tmp/storage-svg` for an isolated preview. The ordinary build and measurements do not need FontTools or font files. Font binaries are not checked in because the finished SVGs are self-contained.

To inspect raster previews separately, install CairoSVG in the same temporary environment and render an SVG:

```bash
/tmp/storage-diagram-tools/bin/pip install 'cairosvg==2.9.1'
/tmp/storage-diagram-tools/bin/cairosvg docs/assets/hybrid-mapping.svg -o /tmp/hybrid-mapping.png
```

Edit the diagram's labels and geometry in the Python generator, then rerender and visually inspect all changed figures. When changing measurements, also check the prose tables against the new JSON. The five checked-in SVGs already contain outlines and need no generation step to display.
