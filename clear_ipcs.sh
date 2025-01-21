#!/bin/bash

lsof | grep Pasazer | grep laskowski | killall Pasazer -9

USERNAME="laskowski"

echo "Czyszczenie zasobów IPC dla użytkownika: $USERNAME"

echo "Usuwanie pamięci współdzielonej..."
ipcs -m | grep $USERNAME | awk '{print $2}' | while read id; do
    echo "Usuwam segment pamięci współdzielonej ID: $id"
    ipcrm -m $id
done

echo "Usuwanie kolejek komunikatów..."
ipcs -q | grep $USERNAME | awk '{print $2}' | while read id; do
    echo "Usuwam kolejkę komunikatów ID: $id"
    ipcrm -q $id
done

echo "Usuwanie semaforów..."
ipcs -s | grep $USERNAME | awk '{print $2}' | while read id; do
    echo "Usuwam semafor ID: $id"
    ipcrm -s $id
done

echo "Wszystkie zasoby IPC dla użytkownika $USERNAME zostały wyczyszczone."
