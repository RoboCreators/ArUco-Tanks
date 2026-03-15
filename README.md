# ArUco Tanks

Dieses Repository enthält die im Rahmen eines Studienprojekts an der HTWK Leipzig entwickelten Software- und CAD-Dateien für ein kamerabasiertes Laser-Tag-Spielsystem mit ferngesteuerten Fahrzeugen.  
Die Treffererkennung erfolgt softwareseitig über ArUco-Marker mittels Computer Vision.

## Inhalt des Repositories

### CAD
- Konstruktionsdaten der Fahrzeugkomponenten
- Gehäuse, Turm und Halterungen

### Software

#### Arduino Steuerungscode
- Fahransteuerung der Fahrzeuge (Servos / Schrittmotor)
- WLAN-Kommunikation mit der Spielumgebung
- Verarbeitung der Controllerdaten

#### Jupyter Notebooks
- Spielumgebung und Visualisierung / Treffererkennung und Spielmechanik
- Verbindungslogik zwischen Laptop und Fahrzeugen
- Erstellung von ArUco-Markern / Kamerakalibrierung und Positionsbestimmung


## Systemübersicht

Das Spielsystem basiert auf der markerbasierten Objekterkennung einer externen Kamera.  
Ein Laptop übernimmt die Bildverarbeitung sowie die Spiellogik und kommuniziert drahtlos mit den ESP32-basierten Fahrzeugen.

## Kontext

Das Projekt wurde im Rahmen des Moduls  
**„Ausgewählte Themen der Automatisierungstechnik“**  
an der **HTWK Leipzig** im Jahr 2025/26 entwickelt.

## Lizenz

Dieses Projekt steht unter der MIT License.
