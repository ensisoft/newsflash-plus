
<?
  include("common.php");
  include("database.php");
  include("version.php");
  
  $platform    = sql_string($_REQUEST['platform']);
  $version     = sql_string($_REQUEST['version']);
  $fingerprint = sql_string($_REQUEST['fingerprint']);
  $host        = sql_string(get_host_name());
  
  $records = mysql_query("SELECT count from newsflash2 where fingerprint = $fingerprint");
  $count   = 0;
  while ($row = mysql_fetch_array($records))
      $count = $row['count'];
	
  ++$count;

  // we cannot use count=count+1 here because that doesn't seem to work with the default value/REPLACE INTO
  // so therefore we first have to select the value and then set it.

  mysql_query("REPLACE INTO newsflash2 SET fingerprint=$fingerprint, host=$host, version=$version, platform=$platform, count=$count", $db) or die($DATABASE_ERROR);
  //mysql_query("UPDATE newsflash SET count=$count where host=$host", $db) or die($DATABASE_ERROR);
  mysql_close($db);
  
  //echo($NEWSFLASH_VERSION);
  echo($NEWSFLASH_VERSION_DEV);

  //echo("\r\n");
  //echo($count);
?>
