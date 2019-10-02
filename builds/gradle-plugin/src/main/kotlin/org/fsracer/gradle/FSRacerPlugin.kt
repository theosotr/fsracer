package org.fsracer.gradle

import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.api.Task


class FSRacerPlugin : Plugin<Project> {
    override fun apply(project: Project) {
        project.allprojects { p ->
            p.gradle.taskGraph.whenReady { taskGraph ->
                println("Begin MAIN " + p.name)
                taskGraph.allTasks.forEach { task ->
                    println("newEvent " + task.name +" W 1")
                    task.getTaskDependencies()
                      .getDependencies(task)
                      .forEach { d ->
                          println("link " + d.name + " " + task.name)
                    }
                    task.doFirst {
                        println("Begin " + task.name)
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
