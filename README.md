# LogRest

Logging measurements to a REST aware Web-Service which writes them to a file

* Measurements are done with a GY-21 sensor
* Using an Arduino Nano 33 IoT 
* `systemd` is configured on the webserver to provide a socket, with connected `gunicorn` service
* python is used for the server code

Have Fun!
Rabnab