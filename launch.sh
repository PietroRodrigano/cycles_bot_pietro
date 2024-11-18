export CYCLES_PORT=50011

cat<<EOF> config.yaml
gameHeight: 500
gameWidth: 500
gameBannerHeight: 100
gridHeight: 100
gridWidth: 100
maxClients: 60
enablePostProcessing: false
EOF

./build/bin/server &
sleep 1

for i in {1..4}
do
./build/bin/client randomio$i &

done
./build/bin/client_Pietro pietro &
