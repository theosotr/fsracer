package org.fsracer.gradle

import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.api.Task

fun constructTaskName(task : Task) : String {
    return ":" + task.project.name + ":" + task.name
}

class FSRacerPlugin : Plugin<Project> {
    override fun apply(project: Project) {
        project.allprojects { p ->
            p.gradle.taskGraph.whenReady { taskGraph ->
                println("Begin MAIN " + p.name)
                taskGraph.allTasks.forEach { task ->
                    val taskName = constructTaskName(task)
                    println("newEvent " + taskName +" W 1")
                    task.getTaskDependencies()
                      .getDependencies(task)
                      .forEach { d ->
                          val depTask = constructTaskName(d)
                          println("link " + depTask + " " + taskName)
                    }
                    task.doFirst {
                        println("Begin " + taskName)
                    }
                    task.doLast {
                        println("End")
                    }
               }
               println("End")
            }
        }
    }
}
