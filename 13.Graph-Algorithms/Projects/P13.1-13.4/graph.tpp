/**
 * @file graph.tpp
 * @brief Template implementation of the graph class declared in graph.hpp.
 *
 * This file is not meant to be included directly; include graph.hpp instead.
 */

namespace graph_adt {

// ------------------------------------------------------------ construction --

template <typename V, typename E>
template <typename VIt, typename EIt, typename>
graph<V, E>::graph(VIt vertex_first, VIt vertex_last, EIt edge_first,
                   EIt edge_last) {
  // Temporary element -> handle map, only needed while resolving the edges.
  std::unordered_map<V, vertex_handle> by_element;
  for (; vertex_first != vertex_last; ++vertex_first) {
    V value = *vertex_first;
    if (by_element.find(value) != by_element.end())
      throw std::invalid_argument("graph: duplicate vertex element");
    const vertex_handle h = insert_vertex(value);
    by_element.emplace(std::move(value), h);
  }
  for (; edge_first != edge_last; ++edge_first) {
    const auto &pair_like = *edge_first;
    const V &u = std::get<0>(pair_like);
    const V &w = std::get<1>(pair_like);
    const auto iu = by_element.find(u);
    const auto iw = by_element.find(w);
    if (iu == by_element.end() || iw == by_element.end())
      throw std::invalid_argument("graph: edge endpoint is not a vertex");
    insert_edge(iu->second, iw->second); // rejects self-loops
  }
}

// -------------------------------------------------------------- collections --

template <typename V, typename E>
std::vector<vertex_handle> graph<V, E>::vertices() const {
  std::vector<vertex_handle> out;
  out.reserve(vertices_.size());
  for (const vertex_node &n : vertices_)
    out.push_back(make_handle(n));
  return out;
}

template <typename V, typename E>
std::vector<edge_handle> graph<V, E>::edges() const {
  std::vector<edge_handle> out;
  out.reserve(edges_.size());
  for (const edge_node &n : edges_)
    out.push_back(make_handle(n));
  return out;
}

template <typename V, typename E>
bool graph<V, E>::contains(vertex_handle v) const {
  return vertex_lookup_.find(v.id()) != vertex_lookup_.end();
}

template <typename V, typename E>
bool graph<V, E>::contains(edge_handle e) const {
  return edge_lookup_.find(e.id()) != edge_lookup_.end();
}

template <typename V, typename E>
std::optional<vertex_handle> graph<V, E>::find_vertex(const V &value) const {
  for (const vertex_node &n : vertices_)
    if (n.element == value)
      return make_handle(n);
  return std::nullopt;
}

// ----------------------------------------------------------------- elements --

template <typename V, typename E>
const V &graph<V, E>::element(vertex_handle v) const {
  return vnode(v).element;
}

template <typename V, typename E>
const E &graph<V, E>::element(edge_handle e) const {
  return enode(e).element;
}

template <typename V, typename E>
V graph<V, E>::replace(vertex_handle v, V value) {
  vertex_node &n = vnode(v);
  V old = std::move(n.element);
  n.element = std::move(value);
  return old;
}

template <typename V, typename E>
E graph<V, E>::replace(edge_handle e, E value) {
  edge_node &n = enode(e);
  E old = std::move(n.element);
  n.element = std::move(value);
  return old;
}

// ------------------------------------------------------------- edge access --

template <typename V, typename E>
std::pair<vertex_handle, vertex_handle>
graph<V, E>::end_vertices(edge_handle e) const {
  const edge_node &n = enode(e);
  return {make_handle(*n.first), make_handle(*n.second)};
}

template <typename V, typename E>
vertex_handle graph<V, E>::origin(edge_handle e) const {
  const edge_node &n = enode(e);
  if (!n.directed)
    throw std::invalid_argument("graph: edge is undirected");
  return make_handle(*n.first);
}

template <typename V, typename E>
vertex_handle graph<V, E>::destination(edge_handle e) const {
  const edge_node &n = enode(e);
  if (!n.directed)
    throw std::invalid_argument("graph: edge is undirected");
  return make_handle(*n.second);
}

template <typename V, typename E>
vertex_handle graph<V, E>::opposite(vertex_handle v, edge_handle e) const {
  const vertex_node &vn = vnode(v);
  const edge_node &n = enode(e);
  if (&vn == n.first)
    return make_handle(*n.second);
  if (&vn == n.second)
    return make_handle(*n.first);
  throw std::invalid_argument("graph: vertex is not an endpoint of the edge");
}

template <typename V, typename E>
bool graph<V, E>::is_directed(edge_handle e) const {
  return enode(e).directed;
}

// ------------------------------------------------------------ vertex access --

template <typename V, typename E>
std::size_t graph<V, E>::degree(vertex_handle v) const {
  return vnode(v).incident.size();
}

template <typename V, typename E>
std::size_t graph<V, E>::in_degree(vertex_handle v) const {
  return vnode(v).in_degree;
}

template <typename V, typename E>
std::size_t graph<V, E>::out_degree(vertex_handle v) const {
  return vnode(v).out_degree;
}

template <typename V, typename E>
std::vector<edge_handle> graph<V, E>::incident_edges(vertex_handle v) const {
  const vertex_node &n = vnode(v);
  std::vector<edge_handle> out;
  out.reserve(n.incident.size());
  for (const edge_node *e : n.incident)
    out.push_back(make_handle(*e));
  return out;
}

template <typename V, typename E>
std::vector<edge_handle> graph<V, E>::out_edges(vertex_handle v) const {
  const vertex_node &n = vnode(v);
  std::vector<edge_handle> out;
  out.reserve(n.out_degree);
  for (const edge_node *e : n.incident)
    if (!e->directed || e->first == &n)
      out.push_back(make_handle(*e));
  return out;
}

template <typename V, typename E>
std::vector<edge_handle> graph<V, E>::in_edges(vertex_handle v) const {
  const vertex_node &n = vnode(v);
  std::vector<edge_handle> out;
  out.reserve(n.in_degree);
  for (const edge_node *e : n.incident)
    if (!e->directed || e->second == &n)
      out.push_back(make_handle(*e));
  return out;
}

template <typename V, typename E>
std::vector<vertex_handle>
graph<V, E>::adjacent_vertices(vertex_handle v) const {
  const vertex_node &n = vnode(v);
  std::vector<vertex_handle> out;
  out.reserve(n.incident.size());
  for (const edge_node *e : n.incident)
    out.push_back(make_handle(e->first == &n ? *e->second : *e->first));
  return out;
}

template <typename V, typename E>
std::vector<vertex_handle>
graph<V, E>::out_adjacent_vertices(vertex_handle v) const {
  const vertex_node &n = vnode(v);
  std::vector<vertex_handle> out;
  out.reserve(n.out_degree);
  for (const edge_node *e : n.incident)
    if (!e->directed || e->first == &n)
      out.push_back(make_handle(e->first == &n ? *e->second : *e->first));
  return out;
}

template <typename V, typename E>
std::vector<vertex_handle>
graph<V, E>::in_adjacent_vertices(vertex_handle v) const {
  const vertex_node &n = vnode(v);
  std::vector<vertex_handle> out;
  out.reserve(n.in_degree);
  for (const edge_node *e : n.incident)
    if (!e->directed || e->second == &n)
      out.push_back(make_handle(e->first == &n ? *e->second : *e->first));
  return out;
}

template <typename V, typename E>
bool graph<V, E>::are_adjacent(vertex_handle v, vertex_handle w) const {
  const vertex_node &a = vnode(v);
  const vertex_node &b = vnode(w);
  if (&a == &b)
    return false;
  // Scan the shorter incidence list: any edge joining a and b appears in both.
  const bool scan_a = a.incident.size() <= b.incident.size();
  const vertex_node &scan = scan_a ? a : b;
  const vertex_node &other = scan_a ? b : a;
  for (const edge_node *e : scan.incident)
    if (e->first == &other || e->second == &other)
      return true;
  return false;
}

template <typename V, typename E>
std::optional<edge_handle> graph<V, E>::get_edge(vertex_handle v,
                                                 vertex_handle w) const {
  const vertex_node &a = vnode(v);
  const vertex_node &b = vnode(w);
  if (&a == &b)
    return std::nullopt;
  // An edge is traversable a -> b if it is stored as (a, b), or if it is
  // undirected and stored as (b, a).
  const auto traversable = [&a, &b](const edge_node *e) {
    return (e->first == &a && e->second == &b) ||
           (!e->directed && e->first == &b && e->second == &a);
  };
  const vertex_node &scan = a.incident.size() <= b.incident.size() ? a : b;
  for (const edge_node *e : scan.incident)
    if (traversable(e))
      return make_handle(*e);
  return std::nullopt;
}

// ------------------------------------------------------------------ updates --

template <typename V, typename E>
vertex_handle graph<V, E>::insert_vertex(V value) {
  const std::uint64_t id = detail::next_object_id();
  vertices_.emplace_back(id, std::move(value));
  detail::rollback_guard undo([this] { vertices_.pop_back(); });
  vertex_lookup_.emplace(id, std::prev(vertices_.end()));
  undo.dismiss();
  return vertex_handle(id);
}

template <typename V, typename E>
edge_handle graph<V, E>::link_edge(vertex_handle v, vertex_handle w, E value,
                                   bool directed) {
  vertex_node &a = vnode(v);
  vertex_node &b = vnode(w);
  if (&a == &b)
    throw std::invalid_argument("graph: self-loops are not supported");

  const std::uint64_t id = detail::next_object_id();
  edges_.emplace_back(id, std::move(value), &a, &b, directed);
  // Each step is undone (in reverse order) if a later step throws, which
  // gives the strong guarantee.
  detail::rollback_guard undo_edge([this] { edges_.pop_back(); });
  edge_node &n = edges_.back();
  n.first_pos = a.incident.insert(a.incident.end(), &n);
  detail::rollback_guard undo_first(
      [&a, &n] { a.incident.erase(n.first_pos); });
  n.second_pos = b.incident.insert(b.incident.end(), &n);
  detail::rollback_guard undo_second(
      [&b, &n] { b.incident.erase(n.second_pos); });
  edge_lookup_.emplace(id, std::prev(edges_.end()));
  undo_second.dismiss();
  undo_first.dismiss();
  undo_edge.dismiss();

  count_edge(n);
  return edge_handle(id);
}

template <typename V, typename E>
edge_handle graph<V, E>::insert_edge(vertex_handle v, vertex_handle w,
                                     E value) {
  return link_edge(v, w, std::move(value), false);
}

template <typename V, typename E>
edge_handle graph<V, E>::insert_directed_edge(vertex_handle v, vertex_handle w,
                                              E value) {
  return link_edge(v, w, std::move(value), true);
}

template <typename V, typename E> E graph<V, E>::remove_edge(edge_handle e) {
  edge_node &n = enode(e);
  E value = std::move(n.element);
  erase_edge(n);
  return value;
}

template <typename V, typename E>
V graph<V, E>::remove_vertex(vertex_handle v) {
  vertex_node &n = vnode(v);
  V value = std::move(n.element);
  // erase_edge also removes the edge from n.incident, so this terminates after
  // d(v) iterations.
  while (!n.incident.empty())
    erase_edge(*n.incident.front());
  const auto map_it = vertex_lookup_.find(n.id);
  const auto list_it = map_it->second;
  vertex_lookup_.erase(map_it);
  vertices_.erase(list_it);
  return value;
}

template <typename V, typename E>
void graph<V, E>::reverse_direction(edge_handle e) {
  edge_node &n = enode(e);
  if (!n.directed)
    throw std::invalid_argument("graph: edge is undirected");
  uncount_edge(n);
  swap_endpoints(n);
  count_edge(n);
}

template <typename V, typename E>
void graph<V, E>::set_direction_from(edge_handle e, vertex_handle v) {
  edge_node &n = enode(e);
  const vertex_node &vn = vnode(v);
  if (&vn != n.first && &vn != n.second)
    throw std::invalid_argument("graph: vertex is not an endpoint of the edge");
  uncount_edge(n);
  if (&vn == n.second)
    swap_endpoints(n); // make v the origin (`first`)
  n.directed = true;
  count_edge(n);
}

template <typename V, typename E>
void graph<V, E>::set_direction_to(edge_handle e, vertex_handle v) {
  edge_node &n = enode(e);
  const vertex_node &vn = vnode(v);
  if (&vn != n.first && &vn != n.second)
    throw std::invalid_argument("graph: vertex is not an endpoint of the edge");
  uncount_edge(n);
  if (&vn == n.first)
    swap_endpoints(n); // make v the destination (`second`)
  n.directed = true;
  count_edge(n);
}

template <typename V, typename E>
void graph<V, E>::make_undirected(edge_handle e) {
  edge_node &n = enode(e);
  uncount_edge(n);
  n.directed = false;
  count_edge(n);
}

} // namespace graph_adt
