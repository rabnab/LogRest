��# LogRest

LogRest uploads the temperature and humidity measurment from the GY-21 sensors up to a webserver via a REST interface.

* The project uses an Arduino NaNo 33 IoT and its WiFi Module
* The REST server uses `gunicorn`and python code 
* a systemd socket which is connected with a service starting gunicorn ensures, that nginx can always access the REST server

