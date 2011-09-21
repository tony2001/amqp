--TEST--
AMQPExchange constructor
--SKIPIF--
<?php if (!extension_loaded("amqp")) print "skip"; ?>
--FILE--
<?php
$cnn = new AMQPConnection();
$cnn->connect();
$ex = new AMQPExchange($cnn);
echo get_class($ex);
?>
--EXPECT--
AMQPExchange