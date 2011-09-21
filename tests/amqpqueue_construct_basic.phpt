--TEST--
AMQPQueue constructor
--SKIPIF--
<?php if (!extension_loaded("amqp")) print "skip"; ?>
--FILE--
<?php
$cnn = new AMQPConnection();
$cnn->connect();
$queue = new AMQPQueue($cnn);
echo get_class($queue);
?>
--EXPECT--
AMQPQueue