package org.fsracer.gradle

import java.io.File

import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.api.Task
import org.gradle.api.tasks.TaskDependency


fun constructTaskName(task : Task) : String =
    "${task.project.name}:${task.name}"


const val GRADLE_PREFIX = "##GRADLE##"


class FSRacerPlugin : Plugin<Project> {
    companion object {
        // This object is used to keep the state of instrumentation.
        var state = State()
    }

    fun processTaskDependencies(taskName: String, task: Task,
                                taskDependencies: TaskDependency) =
        taskDependencies
          .getDependencies(task)
          .forEach { d ->
              val depTask = constructTaskName(d)
              state.addNode(depTask)
              state.addEdge(depTask, taskName)
              println("${GRADLE_PREFIX} dependsOn ${taskName} ${depTask}")
        }

    fun processTaskBegin(task: Task) {
        val taskName = constructTaskName(task)
        state.addNode(taskName)
        println("${GRADLE_PREFIX} newTask ${taskName} W 1")
        task.inputs.files.forEach { input ->
            println("${GRADLE_PREFIX} consumes ${taskName} ${input.absolutePath}")
        }
        task.outputs.files.forEach { output ->
            println("${GRADLE_PREFIX} produces ${taskName} ${output.absolutePath}")
        }
        processTaskDependencies(taskName, task, task.getTaskDependencies())
        processTaskDependencies(taskName, task, task.getMustRunAfter())
        println("${GRADLE_PREFIX} Begin ${taskName}")
    }

    fun processTaskEnd(task: Task) {
        val taskName = constructTaskName(task)
        println("${GRADLE_PREFIX} End ${taskName}")
    }

    override fun apply(project: Project) {
        project.gradle.buildFinished {buildResult ->
            // When the build finishes, store the result of the build
            // in the `build-result.txt` file
            when (buildResult.failure) {
                null -> println("${GRADLE_PREFIX} BUILD ENDED 0")
                else -> println("${GRADLE_PREFIX} BUILD ENDED 1")
            }
            File("build-result.txt").printWriter().use {out ->
                when (buildResult.failure) {
                    null -> out.println("success")
                    else -> out.println(buildResult.failure)
                }
            }
        }
        project.gradle.taskGraph.whenReady { taskGraph ->
            state.addNode("${project.name}:")
            taskGraph.allTasks.forEach { task ->
                task.doFirst {
                    processTaskBegin(task)
                }
                task.doLast {
                    processTaskEnd(task)
                }
           }
           state.toDot()
        }
    }
}
