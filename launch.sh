export CYCLES_PORT=50022

cat<<EOF> config.yaml
gameHeight: 1000
gameWidth: 1000
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
