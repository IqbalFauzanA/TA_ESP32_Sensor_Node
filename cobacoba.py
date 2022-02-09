class sensor:
	parameters = []
	def __init__(self, sensorName, parameter):
		self.sensorName = sensorName
		self.parameters.append(parameter)
    
class parameter:
	def __init__(self, name, value):
		self.name = name
		self.value = value


acid_buffer = parameter("Acid Buffer", 2000)

sensors = [] #array of sensor
sensors.append(sensor("EC", acid_buffer))

print(sensors[0].sensorName)
print(sensors[0].parameters[0].name)
print(sensors[0].parameters[0].value)