package org.fsracer.gradle

import org.gradle.api.Plugin
import org.gradle.api.Project


class FSRacerPlugin : Plugin<Project> {
    override fun apply(project: Project) {
        project.allprojects { p ->
            p.gradle.taskGraph.whenReady { taskGraph ->
                taskGraph.allTasks.forEach { task ->
                    println(task)
                    task.doFirst {
                        println("Begin" + task.name)
                    }
                    task.doLast {
                        println("End")
                    }
                }
            }
        }
    }
}
