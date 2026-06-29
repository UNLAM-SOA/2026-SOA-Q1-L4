# Reglas ProGuard. El proyecto se compila en modo debug sin ofuscacion, por lo
# que no se necesitan reglas especiales. Se conservan las clases de Paho por las
# dudas si en el futuro se activa minifyEnabled en release.
-keep class org.eclipse.paho.** { *; }
-dontwarn org.eclipse.paho.**
