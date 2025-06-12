# Instrucciones de uso

> 📌 Por favor, leer **todas las instrucciones** antes de correr el código para evitar errores.

Abrir al menos **dos terminales en paralelo**. Pueden estar en la misma computadora o en diferentes máquinas conectadas a la misma red.

Una terminal funcionará como **servidor** y las demás como **clientes**.

## 🖥️ Servidor

En la terminal del servidor:

```
cd executables
./server
```

> ⚠️ Es importante que el servidor se ejecute **primero**, ya que debe estar escuchando conexiones entrantes antes de que los clientes se conecten.

## 🧪 Cliente

En cada terminal cliente:

```
cd executables
./client -ip <ip_del_server>
```

- Podés obtener la IP del servidor ejecutando en su terminal:

```
hostname -I
```

> ⚠️ Si no se especifica `-ip`, se usará por defecto la IP **localhost**.

## ⚙️ Configuración opcional del cliente

El código del cliente permite configurar las pruebas que se quieren realizar. Por defecto, se ejecutan todas: `idle`, `download`, `upload` y el envío del archivo JSON.

Podés desactivar alguna pasando argumentos específicos al ejecutar el comando. Por ejemplo, para desactivar el envío del archivo JSON:

```
./client -ip <ip_del_server> -json false
```

### Opciones disponibles:

| Flag         | Descripción                                           | Valor por defecto      |
|--------------|-------------------------------------------------------|------------------------|
| `-idle`      | Ejecutar prueba de inactividad                        | `true`                 |
| `-download`  | Ejecutar prueba de descarga                           | `true`                 |
| `-upload`    | Ejecutar prueba de subida                             | `true`                 |
| `-json`      | Enviar resultados en formato JSON                     | `true`                 |
| `-ip_hostremoto`| IP de destino para enviar el JSON                     | IP del servidor        |
| `-port_host`  | Puerto para el envío del JSON                         | `9999` (puerto Logstash) |

> ⚠️ Si no se asigna una IP de destino para el envío del JSON, se usará la IP del servidor.  
> ⚠️ Si no se especifica el puerto para el JSON, se usará el **puerto 9999** por defecto.

## 🛑 Finalización

Una vez que todos los clientes hayan terminado de enviar los datos, finalizarán automáticamente y se verán los resultados por consola.

El servidor se puede detener manualmente con `Ctrl+C` cuando ya no se esperen más clientes.

## ✅ Ejemplo completo

```
./client -ip 192.168.0.5 -download true -upload true -json false
```

## 📝 Notas finales

- Asegurarse de que todos los ejecutables estén dentro de la carpeta `executables`.
- Verificar que no haya firewalls ni configuraciones de red que bloqueen las conexiones.
