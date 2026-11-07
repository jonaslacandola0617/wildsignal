# Opt-in painted form lighting

Textures that already draw a surface's light and shade can use this material metadata:

```json
"extras": {
  "wildsignal": {
    "painted_form": true
  }
}
```

The boolean defaults to false. Unknown third-party extras are ignored; a supplied
`wildsignal` namespace must be an object and `painted_form` must be a boolean.
The audit tool exposes the parsed flag in each `material_data` entry.

In the pilot dream look's hue-preserving imported-mesh path (family 2), tagged
materials retain the existing palette grade, use the existing shadow visibility
and tint, and receive only a small additional normal-light response. This lets
the texture supply its principal form bands instead of adding strong geometric
triangle bands over them. The normal response uses the supplied surface normal;
geometry, source normals, outlines, shadows and their bias policy are unchanged.
The opt-in is part of the material pool key and is uploaded at each material
switch so differently tagged surfaces cannot share the wrong lighting policy.

This does not change foliage/terrain/rock palette families, the source-material
view, or other rendering paths. It does not make a painted texture dynamically
relit: authored light direction must still be reviewed when sun or object
orientation changes. It is neither an unlit nor an emissive material.

Private visual review uses an unchanged model/texture with matched engine
cameras and toggles only this metadata. Review evidence belongs in the private
art repository. No source artwork is required by the synthetic importer tests.
