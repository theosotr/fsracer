package org.fsracer.gradle

import java.io.File

import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.api.Task


fun constructTaskName(task : Task) : String =
    "${task.project.name}:${task.name}"


class FSRacerPlugin : Plugin<Project> {
    companion object {
        // This object is used to keep the state of instrumentation.
        var state = State()
    } 

    override fun apply(project: Project) {
        project.gradle.buildFinished {buildResult ->
            // When the build finishes, store the result of the build
            // in the `build-result.txt` file
            File("build-result.txt").printWriter().use {out ->
                when (buildResult.failure) {
                    null -> out.println("success")
                    else -> out.println(buildResult.failure)
                }
            }
        }
        project.gradle.taskGraph.whenReady { taskGraph ->
            println("Begin ${project.name}:")
            state.addNode("${project.name}:")
            taskGraph.allTasks.forEach { task ->
                val taskName = constructTaskName(task)
                state.addNode(taskName)
                println("newEvent ${taskName} W 1")
                task.inputs.files.forEach { input ->
                    println("input ${input.absolutePath}")
                }
                task.outputs.files.forEach { output ->
                    println("output ${output.absolutePath}")
                }
                task.getTaskDependencies()
                  .getDependencies(task)
                  .forEach { d ->
                      val depTask = constructTaskName(d)
                      state.addNode(depTask)
                      state.addEdge(depTask, taskName)
                      println("link ${depTask} ${taskName}")
                }
                task.doFirst {
                    println("Begin ${taskName}")
                }
                task.doLast {
                    println("End ${taskName}")
                }
           }
           println("End ${project.name}")
           state.toDot()
        }
    }
}
