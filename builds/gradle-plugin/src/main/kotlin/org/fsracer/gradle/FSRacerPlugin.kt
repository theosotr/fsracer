package org.fsracer.gradle

import java.io.File

import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.api.Task


fun constructTaskName(task : Task) : String =
    "${task.project.name}:${task.name}"


const val GRADLE_PREFIX = "##GRADLE##"


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
            println("${GRADLE_PREFIX} newTask ${project.name}: W 1")
            println("${GRADLE_PREFIX} Begin ${project.name}:")
            state.addNode("${project.name}:")
            taskGraph.allTasks.forEach { task ->
                val taskName = constructTaskName(task)
                state.addNode(taskName)
                println("${GRADLE_PREFIX} newTask ${taskName} W 1")
                task.inputs.files.forEach { input ->
                    println("${GRADLE_PREFIX} consumes ${taskName} ${input.absolutePath}")
                }
                task.outputs.files.forEach { output ->
                    println("${GRADLE_PREFIX} produces ${taskName} ${output.absolutePath}")
                }
                task.getTaskDependencies()
                  .getDependencies(task)
                  .forEach { d ->
                      val depTask = constructTaskName(d)
                      state.addNode(depTask)
                      state.addEdge(depTask, taskName)
                      println("${GRADLE_PREFIX} dependsOn ${taskName} ${depTask}")
                }
                task.doFirst {
                    println("${GRADLE_PREFIX} Begin ${taskName}")
                }
                task.doLast {
                    println("${GRADLE_PREFIX} End ${taskName}")
                }
           }
           println("${GRADLE_PREFIX} End ${project.name}:")
           state.toDot()
        }
    }
}
