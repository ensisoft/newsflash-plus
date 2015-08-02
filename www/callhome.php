
<?php
  // this interface PHP script is used by Newsflash Plus 4.0.0 and newer.
  // prior versions have an older interface in newsflash.php

  include("common.php");
  include("database.php");
  include("version.php");
  
  $platform    = sql_string($_REQUEST['platform']);
  $version     = sql_string($_REQUEST['version']);
  $fingerprint = sql_string($_REQUEST['fingerprint']);
  $host        = sql_string(get_host_name());  
    
  mysql_query("INSERT INTO newsflash2 (fingerprint,host,version,platform,count) " .
              "VALUES($fingerprint, $host, $version, $platform, 1) " . 
              "ON DUPLICATE KEY UPDATE latest=now(), count=count+1, host=$host, version=$version, platform=$platform", $db) or die($DATABASE_ERROR);
  mysql_close($db);
  
  //echo($NEWSFLASH_VERSION);
  echo($NEWSFLASH_VERSION_DEV);

  //echo("\r\n");
  //echo($count);
?>
