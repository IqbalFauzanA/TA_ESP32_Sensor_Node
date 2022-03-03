from dataclasses import dataclass

@dataclass    
class parameter:
	name: str
	value: float

@dataclass
class sensor:
	sensorName: str
	parameters: list




acid_buffer = parameter("Acid Buffer", 2000)
neutral_buffer = parameter("Neutral Buffer", 1000)
parameters = [acid_buffer, neutral_buffer]


sensors = [] #array of sensor
sensors.append(sensor("EC", parameters))

print(sensors[0].sensorName)
print(sensors[0].parameters[0].name)
print(sensors[0].parameters[0].value)
print(sensors[0].parameters[1].name)
print(sensors[0].parameters[1].value)