package org.fsracer.gradle

import java.io.File

import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.api.Task
import org.gradle.api.tasks.TaskDependency


fun constructTaskName(task : Task) : String =
    "${task.project.name}:${task.name}"


fun constrctResourceName(resource: String) : String =
    resource.replace(" ", "@@")


const val GRADLE_PREFIX = "##GRADLE##"


class FSRacerPlugin : Plugin<Project> {

    fun processTaskDependencies(taskName: String, task: Task,
                                taskDependencies: TaskDependency,
                                reverse: Boolean) =
        taskDependencies
          .getDependencies(task)
          .forEach { d ->
              val depTask = constructTaskName(d)
              if (reverse) {
                  println("${GRADLE_PREFIX} dependsOn ${depTask} ${taskName}")
              } else {
                  println("${GRADLE_PREFIX} dependsOn ${taskName} ${depTask}")
              }
        }

    fun processTaskBegin(task: Task) {
        val taskName = constructTaskName(task)
        println("${GRADLE_PREFIX} newTask ${taskName} W 1")
        task.inputs.files.forEach { input ->
            val res = constrctResourceName(input.absolutePath)
            println("${GRADLE_PREFIX} consumes ${taskName} ${res}")
        }
        task.outputs.files.forEach { output ->
            val res = constrctResourceName(output.absolutePath)
            println("${GRADLE_PREFIX} produces ${taskName} ${res}")
        }
        processTaskDependencies(taskName, task, task.getTaskDependencies(), false)
        processTaskDependencies(taskName, task, task.getMustRunAfter(), false)
        processTaskDependencies(taskName, task, task.getFinalizedBy(), true)
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
            taskGraph.allTasks.forEach { task ->
                task.doFirst {
                    processTaskBegin(task)
                }
                task.doLast {
                    processTaskEnd(task)
                }
           }
        }
    }
}
