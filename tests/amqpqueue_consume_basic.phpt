--TEST--
AMQPQueue
--SKIPIF--
<?php if (!extension_loaded("amqp")) print "skip"; ?>
--FILE--
<?php
$cnn = new AMQPConnection();
$cnn->connect();

// Declare a new exchange
$ex = new AMQPExchange($cnn);
$ex->declare('exchange1', AMQP_EX_TYPE_FANOUT);

// Create a new queue
$q = new AMQPQueue($cnn);
$queueName = 'queue1' . time();
$q->declare($queueName);

// Bind it on the exchange to routing.key
$ex->bind($queueName, 'routing.*');

// Publish a message to the exchange with a routing key
$ex->publish('message', 'routing.1');
$ex->publish('message2', 'routing.2');
$ex->publish('message3', 'routing.3');

// Read from the queue
$options = array(
 'min' => 1,
 'max' => 10,
 'ack' => false
);
$msgs = $q->consume($options);

foreach ($msgs as $msg) {
    echo $msg["message_body"] . "\n";
}
?>
--EXPECT--
message
message2
message3

