<?php
  include("database.php");
  include("credentials.php");

  $fingerprint = sql_string($_REQUEST['fingerprint']);
  $licensecode = sql_string($_REQUEST['licensecode']);
  $paypalref   = sql_string($_REQUEST['paypalref']);
  $contributor = sql_string($_REQUEST['contributor']);
  $euro        = sql_string($_REQUEST['euro']);
  $magic       = $_REQUEST['magic'];

  if ($magic != $MAGIC)
      die("no permission");

  mysql_query("UPDATE newsflash2 SET license=$licensecode, paypalref=$paypalref, contributor=$contributor, euro=$euro WHERE fingerprint=$fingerprint");
  mysql_close($db);

  echo("success");

?>