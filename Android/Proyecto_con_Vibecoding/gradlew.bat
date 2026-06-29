@rem Gradle startup script for Windows (Gradle wrapper 8.7).
@rem NOTA: requiere gradle\wrapper\gradle-wrapper.jar. Android Studio lo genera
@rem automaticamente al abrir el proyecto (Sync), o con: gradle wrapper --gradle-version 8.7
@if "%DEBUG%"=="" @echo off
set DIRNAME=%~dp0
set APP_HOME=%DIRNAME%
set CLASSPATH=%APP_HOME%gradle\wrapper\gradle-wrapper.jar
if defined JAVA_HOME (set JAVA_EXE=%JAVA_HOME%\bin\java.exe) else (set JAVA_EXE=java.exe)
"%JAVA_EXE%" -Dorg.gradle.appname=gradlew -classpath "%CLASSPATH%" org.gradle.wrapper.GradleWrapperMain %*
