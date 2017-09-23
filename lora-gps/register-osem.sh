#!/bin/sh

# configuration
OSEM_API=https://api.opensensemap.org
OSEM_USER=test@test
OSEM_PASS=asdfasdf
APP_ID=opensensemap-test
DEV_ID=mobile-feinstaub-2

# log in
OSEM_TOKEN=$(curl -H "content-type: application/json" $OSEM_API/users/sign-in -d '{"email":"'$OSEM_USER'","password":"'$OSEM_PASS'"}' | jq .token  | tr -d '"')

# create a new box
boxresult=$(curl -H "content-type: application/json" -H "Authorization: Bearer $OSEM_TOKEN" -XPOST $OSEM_API/boxes -d '{"name":"'$DEV_ID'","model":"luftdaten_sds011_dht22","exposure":"mobile","location":[51.9,7.6],"ttn":{"profile":"lora-serialization","app_id":"'$APP_ID'","dev_id":"'$DEV_ID'","decodeOptions":[{"decoder":"latLng"},{"decoder":"temperature","sensor_title":"Temperatur"},{"decoder":"humidity","sensor_title":"rel. Luftfeuchte"},{"decoder":"temperature","sensor_title":"PM2.5"},{"decoder":"temperature","sensor_title":"PM10"}]}}')

OSEM_BOX=$(echo $boxresult | jq .data._id  | tr -d '"')
OSEM_SENSOR=$(echo $boxresult | jq .data.sensors[1]._id  | tr -d '"')

echo "$OSEM_API/boxes/$OSEM_BOX/locations
