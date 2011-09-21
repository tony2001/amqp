--TEST--
AMQPQueue
--SKIPIF--
<?php if (!extension_loaded("amqp")) print "skip"; ?>
--FILE--
<?php
$cnn = new AMQPConnection();
$cnn->connect();
$ex = new AMQPExchange($cnn);
$exName = 'exchange-' . time();
$qName = 'queue-' . time();
$ex->declare($exName, AMQP_EX_TYPE_DIRECT);
$queue = new AMQPQueue($cnn);
$queue->declare($qName);
var_dump($queue->bind($exName, 'routing.key'));
?>
--EXPECT--
bool(true)
