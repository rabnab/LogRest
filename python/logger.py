import falcon, json, datetime, pytz
import paho.mqtt.client as mqtt

class TemperatureResource(object):
  fileName = "temperature_{0}.dat"
  # mqttBroker = "mqtt.eclipseprojects.io"
  mqttBroker = "127.0.0.1"
  mqttTemplate = "home/{0}/temperature"
  mqttPayloadTemperature = "{{ \"temperature\": {0} }}" 
  client = mqtt.Client("Temperature_Inside_Rakal")

  def init(self):
      try:
          self.client.on_disconnect=self.mqttDisconnected
          self.client.on_connect=self.mqttConnected
          self.client.on_publish=self.mqttPublished
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

  def mqttPublished(self, client, userdata, mid=0):
      print("mqtt published ", flush=True)


  def on_get(self, req, resp):
      if req.path == "/temperature": 
        response = "{ \"request.path\" : " + req.path + ","
        response += "\"request.query\" : " + req.query_string+ ","
        response +="\"answer\": " 

        try:
            fName=self.fileFromRequest(req)
            lb = int(req.get_param("lb",required=True))
            ub = int(req.get_param("ub",required=True))
            msg = ""
            with open(fName, 'r') as theFile:
                cnt=0
                for line in theFile:
                    cnt+=1
                    if cnt>=lb and cnt<ub:
                        msg+=line
            response += msg
        except Exception as ex:
            response += "error -- " + repr(ex)

        response +="}\n"
        resp.body = response

  def on_post(self, req, resp):
      if req.path != "/temperature": 
          return
      try:
          #publish to file
          #self.addToFile(req)
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
      else:
          resp.status = falcon.HTTP_201
          resp.body = json.dumps({"success": True})

  def pubToMqTT(self, dataAsRequest):
      temperatureString=dataAsRequest.get_param("y", required=True, default="nodata")
      temperatureKelvin=dataAsRequest.get_param_as_int("y",min_value=0,max_value=1000000,required=True) 
      #print("temperatureKelvin: ", repr(temperatureKelvin))
      #print("temperatureString: ", repr(temperatureString))
      temperatureCelsius = (temperatureKelvin-273150)/1000
      #temperatureCelsius = temperatureKelvin
      sourceName=dataAsRequest.get_param("loc", required=True)
      mqttTopic = self.mqttTemplate.format(sourceName)
      mqttPayloadTemperatureUsed = self.mqttPayloadTemperature.format(repr(temperatureCelsius))
      #{ \"temperature\": 23.23 }
      print("publishing: " + mqttTopic + " payload: " + mqttPayloadTemperatureUsed, flush=True)
      try:
          pubResult=self.client.publish(mqttTopic, mqttPayloadTemperatureUsed)
          if pubResult.rc!= 0:
            print("pubResult was not succesful: " + mqtt.error_string(pubResult.rc), flush=True)
            client.loop_stop()
            client.connect(self.mqttBroker)
            client.loop_start() 
      except Exception as ex:
          print("mqtt publish resulted in error: " + repr(ex))



  def fileFromRequest(self, dataAsRequest):
      sourceName=dataAsRequest.get_param("loc", required=True)
      tab=sourceName.maketrans("\\/","  ")
      fName = self.fileName.format(sourceName.translate(tab)[:300])
      return fName

  def addToFile(self, dataAsRequest):
      dateAsSecond=dataAsRequest.get_param_as_datetime("x",format_string="%Y-%m-%d %H:%M:%S",required=False)
      if dateAsSecond is None:
         tz=pytz.timezone('Europe/Vienna')
         dateAsSecond =  datetime.datetime.now(tz)
      temperatureKelvin=dataAsRequest.get_param_as_int("y",min_value=0,max_value=1000000,required=True) 
      state=dataAsRequest.get_param("s", required=False, default="g")[:1]
      fName=self.fileFromRequest(dataAsRequest)
      with open(fName, 'a') as theFile:
          lineFormat = "{0};{1};{2}\n".format(dateAsSecond.isoformat(timespec='seconds'), (temperatureKelvin-273150), state)
          theFile.write(lineFormat)

api = falcon.API()
print("start up REST api logger", flush=True)
temperature_endpoint = TemperatureResource()
temperature_endpoint.init()
print("initialized REST api logger", flush=True)
api.add_route('/temperature', temperature_endpoint)
print("added endpoint for temperatures", flush=True)
