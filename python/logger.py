import falcon, json, datetime, pytz
import paho.mqtt.client as mqtt
import traceback
import loggerPhysics

class MeasurementResource(object):
  mqttBroker = "127.0.0.1"
  mqttMeasTopic = "home/{0}/meas"
  client = mqtt.Client("Temperature_Inside_Rakal")
  deviceIdentifier = {'SZ': '["0afe"]', 'WZ': '["0afd"]', 'test': '["1234"]'}
  deviceName = {'SZ': 'Schlafzimmer Sensor', 'WZ': 'Wohnzimmer Sensor', 'test': 'forgetIt'}
  lstReceivedLocations = []

  def init(self):
      try:
          self.client.on_disconnect=self.mqttDisconnected
          self.client.on_connect=self.mqttConnected
          #self.client.on_publish=self.mqttPublished
          self.client.username_pw_set("mosquitto", "mytridil83")
          self.client.connect(self.mqttBroker)
          self.client.loop_start()
          print("connection to mqtt done", flush=True)
      except Exception as ex:
          print("error mqtt:", repr(ex))
          raise Exception("could not establish connection to mqtt broker")


  def mqttConnected(self, client, userdata,flags, rc=0):
      print("mqtt connected rc:"+str(rc), flush=True)

  def mqttDisconnected(self, client, userdata, rc=0):
      print("mqtt disconnected rc:"+str(rc), flush=True)
      client.loop_stop()
      
  def on_post(self, req, resp):
      #print("entering post for path: " + req.path, flush=True)
      if req.path != "/meas":
          return
      #print("accepted path: " + req.path, flush=True)
      try:
          #publish to mqtt
          #print("try posting to mqtt", flush=True)
          self.pubToMqTT(req)
          #print("done posting to mqtt", flush=True)
      except falcon.HTTPBadRequest as httpEx:
          resp.status = falcon.HTTP_BAD_REQUEST
          resp.body = json.dumps({"success": False, "reason": repr(httpEx)})
      except Exception as ex:
          resp.status = falcon.HTTP_NOT_IMPLEMENTED
          resp.body = json.dumps({"success": False, "reason": repr(ex)})
          print("exception while publishing: " + repr(ex), flush=True)
          print("-"*60)
          traceback.print_exc()
          print("-"*60)
      else:
          resp.status = falcon.HTTP_201
          resp.body = json.dumps({"success": True})

  def setupNewLocation(self, loc):
      if loc not in self.lstReceivedLocations:
          if loc in self.deviceIdentifier:
              sensorType = ["temperature","humidity"]
              for curType in sensorType:
                  mqttTopic = "homeassistant/sensor/{1}_{0}/config".format(curType.lower(),loc.lower())
                  
                  mqttPayload = json.dumps({"device_class": curType,
                      "name": "{0}_{1}".format(loc, curType).lower(),
                      "state_topic": "home/{0}/meas".format(loc),
                      "unique_id": "{0}_{1}".format(loc, curType),
                      "value_template":  "{{{{ value_json.HTU21.{0} | round(2) }}}}".format(curType),
                      "unit_of_measurement": ({True: "°C", False: "%"}) [curType == "temperature"] ,
                      "device": {
                          "identifiers": self.deviceIdentifier[loc],
                          "name": self.deviceName[loc]
                          }
                      }, indent=2)
                  print("publishing: " + mqttTopic + " payload: " + mqttPayload, flush=True)
                  pubs(self.client, self.mqttBroker, mqttTopic, mqttPayload)
              self.lstReceivedLocations.append(loc)
          else:
              print("location: " + loc + " not configurable", flush=True)
      #else:
         #print("location: " + loc + " already in list of configured location: " + repr(self.lstReceivedLocations), flush=True)
      
  def pubToMqTT(self, dataAsRequest):
      #print("entering publish: " + repr(dataAsRequest), flush=True)
      temperatureString=dataAsRequest.get_param("t", required=True, default="nodata")
      temperatureKelvin=dataAsRequest.get_param_as_int("t",min_value=0,max_value=1000000,required=True) 
      #print("temperatureKelvin: ", repr(temperatureKelvin))
      #print("temperatureString: ", repr(temperatureString))
      temperatureCelsius = (temperatureKelvin-273150)/1000
      
      humidityString=dataAsRequest.get_param("h", required=True, default="nodata")
      humidityPPM=dataAsRequest.get_param_as_int("h",min_value=0,max_value=2000000,required=True) 


      humidityPerc=humidityPPM/10000;
      
      dewPoint = loggerPhysics.dewPoint(temperatureCelsius, humidityPerc)

      sourceName=dataAsRequest.get_param("loc", required=True)

      self.setupNewLocation(sourceName)
        

      #print("\tmeas from source: " + sourceName, flush=True)
      #print("\ttemperature: " + repr(temperatureCelsius), flush=True)
      #print("\thumidity: " + repr(humidityPerc), flush=True)
      mqttTopic=self.mqttMeasTopic.format(sourceName)
      mqttPayload = json.dumps({'HTU21':{
          'temperature':temperatureCelsius,
          'humidity':humidityPerc,
          'dewpoint':dewPoint,
          }},indent=2) 
      print("publishing: " + mqttTopic + " payload: " + mqttPayload, flush=True)
      pubs(self.client, self.mqttBroker, mqttTopic, mqttPayload)

def pubs(client, mqttBroker, topic, payload):
  try:
      pubResult=client.publish(topic, payload)
      if pubResult.rc!= 0:
        print("pubResult was not succesful: " + mqtt.error_string(pubResult.rc), flush=True)
        client.loop_stop()
        client.connect(mqttBroker)
        client.loop_start() 
  except Exception as ex:
      print("mqtt publish resulted in error: " + repr(ex))


##set backend for authentication
#backend = BasicAuthBackend(basic_loader)
##create middleware interacting with backend
#auth_middleware = AuthMiddleware(backend)
#api = falcon.API(middleware=[auth_middleware])
api = falcon.API()
print("start up REST api logger", flush=True)
logger_endpoint = MeasurementResource()
logger_endpoint.init()
print("initialized REST api logger", flush=True)
api.add_route('/meas', logger_endpoint)
print("added endpoint for measurements", flush=True)
