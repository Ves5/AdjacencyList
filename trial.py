import simple_graphs

g = simple_graphs.AdjacencyList().create_star(5)

#g = simple_graphs.AdjacencyList("ICRdlpzxo")

print(g.vertices())
print(g.number_of_vertices())
print(g.edges())
print(g.add_vertex(10))
print(g.vertices())
print(g.delete_vertex(9))
print(g.edges())
print(g.vertices())