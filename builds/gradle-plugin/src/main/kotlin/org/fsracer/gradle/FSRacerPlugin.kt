package org.fsracer.gradle

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

    override fun apply(project: Project) =
        project.gradle.taskGraph.whenReady { taskGraph ->
            println("Begin MAIN ${project.name}")
            taskGraph.allTasks.forEach { task ->
                val taskName = constructTaskName(task)
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
                      println("link ${depTask} ${taskName}")
                }
                task.doFirst {
                    println("Begin ${taskName}")
                }
                task.doLast {
                    println("End")
                }
           }
           println("End")
        }
}
