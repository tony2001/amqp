--TEST--
AMQPExchange
--SKIPIF--
<?php if (!extension_loaded("amqp")) print "skip"; ?>
--FILE--
<?php
$cnn = new AMQPConnection();
$cnn->connect();

$ex = new AMQPExchange($cnn);
$ex->declare('exchange-' . time());
echo $ex->publish('message', 'routing.key') ? 'true' : 'false';
?>
--EXPECT--
true