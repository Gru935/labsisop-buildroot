#!/bin/sh

cp $BASE_DIR/../custom-scripts/S41network-config $BASE_DIR/target/etc/init.d
chmod +x $BASE_DIR/target/etc/init.d/S41network-config
make -C $BASE_DIR/../modules/xtea_driver/
make -C $BASE_DIR/../modules/sstf/