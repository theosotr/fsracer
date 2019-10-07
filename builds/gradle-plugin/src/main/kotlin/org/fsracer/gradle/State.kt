package org.fsracer.gradle

import java.io.File


class State {
    val depGraph : MutableMap<String, Set<String>>

    init {
        depGraph = mutableMapOf()
    }

    fun addNode(node: String) =
        when (depGraph.contains(node)) {
            true  -> {}
            false -> depGraph.put(node, emptySet())
        }

    fun addEdge(source: String, target: String) {
        val edges = depGraph.get(source)
        if (edges == null) {
            depGraph.put(source, setOf(target))
        } else {
            depGraph.put(source, edges.plus(target))
        }
    }

    fun toDot() =
        File("graph.json").printWriter().use { out ->
            out.println("{")
            out.println("  \"resources\": [")
            var i = 0
            val size = depGraph.keys.size
            depGraph.forEach{node, edges ->
                out.println("    {")
                val segments = node.split(':', limit=2)
                out.println("      \"type\": \"${segments[0]}\",")
                out.println("      \"title\": \"${segments[1]}\",")
                out.println("      \"parameters\": {")
                out.println("        \"before\": [")
                val edgeSize = edges.size
                var j = 0
                edges.forEach{target ->
                    if (j != edgeSize - 1) {
                        out.println("        \"${target}\",")
                    } else {
                        out.println("        \"${target}\"")
                    }
                    j++
                }
                out.println("        ]")
                out.println("      }")
                if (i != size - 1) {
                    out.println("    },")
                } else {
                    out.println("    }")
                }
                i++
            }
            out.println("  ], \"edges\": []")
            out.println("}")
        }
}
