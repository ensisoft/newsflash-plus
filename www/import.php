<?php
  include("common.php");
  include("database.php");

  $platform = sql_string($_REQUEST['platform']);
  $host     = sql_string(get_host_name());
  
  $insert = "INSERT INTO import (platform,  host) VALUES ($platform, $host)";
  
  // ignore any errors
  @mysql_query($insert, $db);
  @mysql_close($db);
  
  $IMPORT_VERSION = 1;
  
  echo($IMPORT_VERSION);
  echo("\n");

  // write the servers with tab as the field delimeter


  // username: newsflash
  // password: newsflash
  //echo("http://nzb.su/api");
  echo("http://nzbsooti.sx/api");
  echo("\t");
  echo("cbff37a3f9eb181de2bce28900a14157");
  echo("\n");

  

  
?>