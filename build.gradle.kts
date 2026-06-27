// Top-level build file where you can add configuration options common to all sub-projects/modules.
buildscript {
    repositories {
        google()
        mavenCentral()
        maven("https://artifacts.applovin.com/android")
    }

    dependencies {
        classpath("com.android.tools.build:gradle:8.13.1")
        classpath("com.google.gms:google-services:4.4.2")
        classpath("com.google.firebase:firebase-crashlytics-gradle:2.9.9")
        classpath("org.jetbrains.kotlin:kotlin-gradle-plugin:2.2.0")
    }
}

allprojects {
    repositories {
        mavenCentral()
        maven("https://jitpack.io")
        maven("https://mint.splunk.com/gradle/")
        maven("https://oss.sonatype.org/content/repositories/snapshots/")
        google()
    }
}