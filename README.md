# RoboTiger 2026

## Guía de GitHub — Sincronización del proyecto

Antes de comenzar, asegúrate de tener instalado [Git](https://git-scm.com/downloads).

Para verificar, abre PowerShell y ejecuta:

```bash
git --version
```

Si esta instalado correctamente, aparecera algo similar a

```text
git version X.XX.X
```

### 1. Descargar el proyecto por primera vez (Iniciar)

Si todavía no tienes el proyecto en tu computadora, abre una terminal en la ubicación donde quieras guardarlo y ejecuta:

```bash
git clone https://github.com/Bort-s/RoboTiger-2026
```

Esto descargará el proyecto completo y creará una carpeta con su contenido.

**Importante:** Este paso solamente se realiza una vez por computadora. Si ya tienes el proyecto, no necesitas volver a clonarlo.

### 2. Actualizar el proyecto (Actualizar)

Cuando el padre actualize el projecto, puedes descargarla y combinarla con tus cambios locales mediante:

```bash
git pull origin main
```

Esto permite:
- Descargar los cambios más recientes.
- Integrarlos con tu versión local.
- Conservar tus modificaciones locales siempre que no existan conflictos.

Si Git informa que existen conflictos, tendran que ser resueltos antes de continuar (usar chatsito o preguntar al padre).

### 3. Restaurar el proyecto (Reestablecer)

Si la cachucheas: modificaste archivos accidentalmente, rompiste algo o simplemente quieres **descartar** tus modificaciones locales y recuperar la versión original, ejecuta:

```bash
git fetch origin
git reset --hard origin/main
git clean -fd
```

Este procedimiento elimina las modificaciones locales. No se pueden recuperar fácilmente.

---

**El repositorio de GitHub contiene la versión oficial del proyecto.** Puedes modificar tu copia local libremente, pero siempre podrás recuperar la última versión publicada por el administrador siguiendo esta guía.