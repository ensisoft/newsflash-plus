<?php
  include("credentials.php");
  $db = mysql_connect($DB_SERVER, $DB_USER, $DB_PASS) or die($DATABASE_UNAVAILABLE);
  mysql_select_db($DB_NAME);
?>