/**
 * @file simple_graph.tpp
 * @brief Template implementation of the simple_graph class declared in simple_graph.hpp.
 *
 * This file is not meant to be included directly; include simple_graph.hpp instead.
 */

namespace graph_adt {

template <typename V, typename Storage, typename Hash, typename KeyEqual>
template <typename VIt, typename EIt, typename>
simple_graph<V, Storage, Hash, KeyEqual>::simple_graph(VIt vertex_first,
                                                       VIt vertex_last,
                                                       EIt edge_first,
                                                       EIt edge_last) {
  for (; vertex_first != vertex_last; ++vertex_first)
    index_.add(*vertex_first); // throws on duplicate vertex elements

  std::vector<edge_index> idx;
  for (; edge_first != edge_last; ++edge_first) {
    const auto &pair_like = *edge_first;
    const V &u = std::get<0>(pair_like);
    const V &v = std::get<1>(pair_like);
    const auto iu = index_.find(u);
    const auto iv = index_.find(v);
    if (!iu || !iv)
      throw std::invalid_argument(
          "simple_graph: edge endpoint is not a vertex");
    if (*iu == *iv)
      throw std::invalid_argument("simple_graph: self-loops are not allowed");
    // Normalise so that first < second: {u, v} and {v, u} are the same edge.
    idx.emplace_back(std::min(*iu, *iv), std::max(*iu, *iv));
  }

  // Sorting gives the lexicographic order every Storage requires and makes
  // duplicate edges adjacent, so they can be detected in one pass.
  std::sort(idx.begin(), idx.end());
  if (std::adjacent_find(idx.begin(), idx.end()) != idx.end())
    throw std::invalid_argument("simple_graph: duplicate edge");

  storage_ = Storage(index_.size(), std::move(idx));
}

template <typename V, typename Storage, typename Hash, typename KeyEqual>
bool simple_graph<V, Storage, Hash, KeyEqual>::are_adjacent(const V &u,
                                                            const V &v) const {
  return storage_.adjacent(index_.at(u), index_.at(v));
}

template <typename V, typename Storage, typename Hash, typename KeyEqual>
std::size_t simple_graph<V, Storage, Hash, KeyEqual>::degree(const V &v) const {
  return storage_.degree(index_.at(v));
}

template <typename V, typename Storage, typename Hash, typename KeyEqual>
std::vector<V>
simple_graph<V, Storage, Hash, KeyEqual>::neighbors(const V &v) const {
  const std::vector<std::size_t> idx = storage_.neighbors(index_.at(v));
  std::vector<V> out;
  out.reserve(idx.size());
  for (const std::size_t i : idx)
    out.push_back(index_.value(i));
  return out;
}

template <typename V, typename Storage, typename Hash, typename KeyEqual>
std::vector<std::pair<V, V>>
simple_graph<V, Storage, Hash, KeyEqual>::edges() const {
  const std::vector<edge_index> idx = storage_.edges();
  std::vector<std::pair<V, V>> out;
  out.reserve(idx.size());
  for (const auto &[i, j] : idx)
    out.emplace_back(index_.value(i), index_.value(j));
  return out;
}

} // namespace graph_adt
