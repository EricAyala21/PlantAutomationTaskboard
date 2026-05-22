
////////////////////////////////////////////////////////////////////////////////////////
// Moisture Sensor class definition/parameters
// Trig pin goes to GIOP ?
////////////////////////////////////////////////////////////////////////////////////////

class MoistureSensor{
    private:
        int MoisturePin;

    public:
        MoistureSensor(int MoisturePin){
            pinMode(MoisturePin,INPUT);
            this->MoisturePin = MoisturePin;

        }
        int mappedValue(){
            return map(analogRead(MoisturePin),975,2672,0,100);
        }
        int sensorAverage(MoistureSensor sensorTwo){
            return(mappedValue() + sensorTwo.mappedValue())/2;
        }

};

class LightSensor{
    private:
        int lightPin;

    public:
        LightSensor(int LightPin){
            pinMode(LightPin,INPUT);
            this->LightPin = LightPin;
        }
        int mapLight(){
            return map(analogRead(lightPin),0,2432,0, 100);
        }
};
