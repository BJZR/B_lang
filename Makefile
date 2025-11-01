# Makefile para el compilador B
# BJZR - Desarrollo simple y limpio

CC = gcc
CFLAGS = -Wall -O2
TARGET = b
SOURCE = main.c

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(SOURCE)
	@echo "Compilando $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)
	@echo "Compilación exitosa: ./$(TARGET)"

clean:
	@echo "Limpiando archivos..."
	rm -f $(TARGET) output.asm output.o program
	@echo "Limpieza completa"

install: $(TARGET)
	@echo "Instalando $(TARGET) en /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "Instalado: puedes usar 'z' desde cualquier lugar"

uninstall:
	@echo "Desinstalando $(TARGET)..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Desinstalado"

test: $(TARGET)
	@echo "Probando el compilador..."
	@if [ -f test.b ]; then \
		./$(TARGET) run test.b; \
	else \
		echo "No se encontró test.b"; \
	fi

help:
	@echo "Makefile para el compilador B"
	@echo ""
	@echo "Comandos disponibles:"
	@echo "  make          - Compila el compilador"
	@echo "  make clean    - Limpia archivos generados"
	@echo "  make install  - Instala en /usr/local/bin"
	@echo "  make uninstall- Desinstala"
	@echo "  make test     - Prueba con test16.zr"
	@echo "  make help     - Muestra esta ayuda"
