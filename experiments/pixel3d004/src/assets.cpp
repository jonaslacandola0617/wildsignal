#include "core.hpp"
#define CGLTF_IMPLEMENTATION
#include <algorithm>
#include <cgltf.h>
#include <memory>
#include <stdexcept>
namespace p3 {
Model Model::load(const std::filesystem::path &path) {
  auto bytes = readText(path, 16 * 1024 * 1024);
  cgltf_options opt{};
  cgltf_data *raw = nullptr;
  if (cgltf_parse(&opt, bytes.data(), bytes.size(), &raw) !=
      cgltf_result_success)
    throw std::runtime_error("invalid glTF: " + path.string());
  std::unique_ptr<cgltf_data, decltype(&cgltf_free)> data(raw, cgltf_free);
  auto &d = *data;
  if (d.skins_count || d.animations_count || d.extensions_required_count ||
      d.meshes_count > 512 || d.nodes_count > 1024 || d.materials_count > 128 ||
      d.buffers_count > 32)
    throw std::runtime_error("unsupported/oversize glTF schema");
  for (std::size_t i = 0; i < d.nodes_count; i++) {
    auto *parent = &d.nodes[i];
    std::size_t hops = 0;
    while (parent) {
      if (++hops > d.nodes_count)
        throw std::runtime_error("cyclic hierarchy");
      parent = parent->parent;
    }
  }
  std::uint64_t total = 0;
  for (std::size_t i = 0; i < d.buffers_count; i++) {
    auto &b = d.buffers[i];
    if (!b.uri || b.size > 128 * 1024 * 1024)
      throw std::runtime_error("external buffer required/limit");
    auto p = safePath(path.parent_path(), b.uri);
    if (std::filesystem::file_size(p) != b.size)
      throw std::runtime_error("buffer size mismatch");
    total += b.size;
  }
  if (total > 256 * 1024 * 1024)
    throw std::runtime_error("geometry limit");
  for (std::size_t i = 0; i < d.images_count; i++) {
    auto &im = d.images[i];
    if (!im.uri || im.buffer_view)
      throw std::runtime_error("external PNG required");
    safePath(path.parent_path(), im.uri);
  }
  if (cgltf_load_buffers(&opt, data.get(), path.string().c_str()) !=
          cgltf_result_success ||
      cgltf_validate(data.get()) != cgltf_result_success)
    throw std::runtime_error("glTF buffer/index validation failed");
  Model out;
  out.path = path;
  for (std::size_t i = 0; i < d.materials_count; i++) {
    auto &m = d.materials[i];
    Material mat;
    mat.name = m.name ? m.name : "unnamed";
    if (!m.has_pbr_metallic_roughness || m.has_transmission || m.has_volume ||
        m.has_clearcoat || m.has_specular || m.has_ior || m.unlit)
      throw std::runtime_error("unsupported material");
    auto &p = m.pbr_metallic_roughness;
    std::copy_n(p.base_color_factor, 4, mat.color.begin());
    for (float x : mat.color)
      if (!std::isfinite(x) || x < 0 || x > 1)
        throw std::runtime_error("material color");
    auto &t = p.base_color_texture;
    if (t.texture) {
      if (t.texcoord || t.has_transform || !t.texture->image ||
          !t.texture->image->uri)
        throw std::runtime_error("unsupported texture view");
      mat.texture = safePath(path.parent_path(), t.texture->image->uri);
      if (mat.texture.extension() != ".png")
        throw std::runtime_error("PNG only");
      if (t.texture->sampler) {
        mat.wrapS = static_cast<int>(t.texture->sampler->wrap_s);
        mat.wrapT = static_cast<int>(t.texture->sampler->wrap_t);
      }
    }
    mat.alpha = m.alpha_mode == cgltf_alpha_mode_mask    ? Alpha::Mask
                : m.alpha_mode == cgltf_alpha_mode_blend ? Alpha::Blend
                                                         : Alpha::Opaque;
    mat.cutoff = m.alpha_cutoff;
    mat.doubleSided = m.double_sided != 0;
    out.materials.push_back(std::move(mat));
  }
  if (out.materials.empty())
    throw std::runtime_error("explicit materials required");
  // Only nodes reachable from the default scene are rendered. Reject
  // instancing, skinning and morphs rather than silently interpreting them as
  // static geometry.
  if (!d.scene || !d.scene->nodes_count)
    throw std::runtime_error("default scene required");
  std::vector<const cgltf_node *> stack;
  for (std::size_t i = 0; i < d.scene->nodes_count; i++)
    stack.push_back(d.scene->nodes[i]);
  std::size_t visited = 0;
  while (!stack.empty()) {
    auto *n = stack.back();
    stack.pop_back();
    if (++visited > d.nodes_count)
      throw std::runtime_error("node cycle/repeated child");
    if (n->skin || n->has_mesh_gpu_instancing)
      throw std::runtime_error("unsupported node");
    for (std::size_t i = 0; i < n->children_count; i++)
      stack.push_back(n->children[i]);
    if (!n->mesh)
      continue;
    Mat world;
    cgltf_node_transform_world(n, world.v.data());
    for (float f : world.v)
      if (!std::isfinite(f))
        throw std::runtime_error("node transform");
    Mat normal = transpose(inverse(world));
    float determinant =
        dot(direction(world, {1, 0, 0}),
            cross(direction(world, {0, 1, 0}), direction(world, {0, 0, 1})));
    for (std::size_t j = 0; j < n->mesh->primitives_count; j++) {
      auto &p = n->mesh->primitives[j];
      if (p.type != cgltf_primitive_type_triangles || p.targets_count ||
          p.has_draco_mesh_compression || !p.material)
        throw std::runtime_error("triangles/static material required");
      auto *pos = cgltf_find_accessor(&p, cgltf_attribute_type_position, 0);
      auto *norm = cgltf_find_accessor(&p, cgltf_attribute_type_normal, 0);
      auto *uv = cgltf_find_accessor(&p, cgltf_attribute_type_texcoord, 0);
      if (!pos || !norm || !uv || pos->type != cgltf_type_vec3 ||
          norm->type != cgltf_type_vec3 || uv->type != cgltf_type_vec2 ||
          pos->count != norm->count || pos->count != uv->count ||
          pos->count > 2000000 || pos->is_sparse || norm->is_sparse ||
          uv->is_sparse)
        throw std::runtime_error("POSITION/NORMAL/TEXCOORD_0 required");
      auto *color = cgltf_find_accessor(&p, cgltf_attribute_type_color, 0);
      if (color &&
          (color->count != pos->count || color->is_sparse ||
           (color->type != cgltf_type_vec3 && color->type != cgltf_type_vec4)))
        throw std::runtime_error("invalid COLOR_0");
      Primitive primitive;
      primitive.material = static_cast<std::size_t>(p.material - d.materials);
      if (primitive.material >= out.materials.size())
        throw std::runtime_error("material reference");
      primitive.vertices.reserve(pos->count);
      for (std::size_t k = 0; k < pos->count; k++) {
        float a[3], b[3], c[2];
        if (!cgltf_accessor_read_float(pos, k, a, 3) ||
            !cgltf_accessor_read_float(norm, k, b, 3) ||
            !cgltf_accessor_read_float(uv, k, c, 2) || !std::isfinite(c[0]) ||
            !std::isfinite(c[1]))
          throw std::runtime_error("attribute read");
        Vertex v{point(world, {a[0], a[1], a[2]}),
                 normalized(direction(normal, {b[0], b[1], b[2]})), c[0], c[1]};
        if (color) {
          float rgba[4] = {1, 1, 1, 1};
          auto components = color->type == cgltf_type_vec3 ? 3u : 4u;
          if (!cgltf_accessor_read_float(color, k, rgba, components))
            throw std::runtime_error("COLOR_0 read");
          for (unsigned c = 0; c < components; c++) {
            if (!std::isfinite(rgba[c]) || rgba[c] < -.001f || rgba[c] > 1.001f)
              throw std::runtime_error("COLOR_0 range");
            v.color[c] = std::clamp(rgba[c], 0.f, 1.f);
          }
        }
        primitive.bounds.add(v.p);
        primitive.vertices.push_back(v);
      }
      auto count = p.indices ? p.indices->count : pos->count;
      if (count % 3 || count > 6000000 || (p.indices && p.indices->is_sparse))
        throw std::runtime_error("triangle index count");
      primitive.indices.reserve(count);
      for (std::size_t k = 0; k < count; k++) {
        auto ix = p.indices ? cgltf_accessor_read_index(p.indices, k) : k;
        if (ix >= primitive.vertices.size())
          throw std::runtime_error("index out of range");
        primitive.indices.push_back(static_cast<std::uint32_t>(ix));
      }
      if (determinant < 0)
        for (std::size_t k = 0; k < count; k += 3)
          std::swap(primitive.indices[k + 1], primitive.indices[k + 2]);
      out.bounds.add(primitive.bounds);
      out.parts.push_back(std::move(primitive));
    }
  }
  if (!out.bounds.valid() || out.parts.empty())
    throw std::runtime_error("empty model");
  return out;
}
} // namespace p3
