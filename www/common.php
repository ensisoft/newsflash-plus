
<?php
  $SUCCESS              = "0";
  $DIRTY_ROTTEN_SPAMMER = "1";
  $DATABASE_UNAVAILABLE = "2";
  $DATABASE_ERROR       = "3";
  $EMAIL_UNAVAILABLE    = "4";
  $ERROR_QUERY_PARAMS   = "5";

  $TYPE_NEGATIVE_FEEDBACK = 1;
  $TYPE_POSITIVE_FEEDBACK = 2;
  $TYPE_NEUTRAL_FEEDBACK  = 3;
  $TYPE_BUG_REPORT        = 4;
  $TYPE_FEATURE_REQUEST   = 5;
  $TYPE_LICENSE_REQUEST   = 6;

  $MY_EMAIL             = "samiv@ensisoft.com";

  function get_host_name()
  {
      $ip = getenv("HTTP_CLIENT_IP");
      if (strlen($ip) == 0)
      {
          $ip = getenv("HTTP_X_FORWARDED_FOR");
          if (strlen($ip) == 0)
              $ip = getenv("REMOTE_ADDR");
      }
      if (strlen($ip))
          return gethostbyaddr($ip);
      return "unknown";
  }
  
  function sql_string($str)
  {
      if (isset($str))
          return "'" . addslashes(strip_tags($str)) . "'";
      // this is good for SQL embedding
      return "NULL";
  }
  
  function sql_check_spam($tablename, $host)
  {
      // check for flooding. if there are more than 2 messages from
      // the same host within the past 5 minutes posting is ignored
      $records = mysql_query("SELECT id FROM $tablename WHERE UNIX_TIMESTAMP(date) >= UNIX_TIMESTAMP(NOW()) - 300 AND host like $host") or die($DATABASE_ERROR);
      $count   = 0;
      while ($row = mysql_fetch_array($records))
      {
          $count = $count + 1;
          if ($count == 2)
              return True;
      }
      return False;
  }
  
  function create_random_string()
  {
      for ($i=0; $i<6; $i++) 
      {
          $d  = rand(1,30)%2;
          $s .= $d ? chr(rand(65,90)) : chr(rand(48,57));      
      }
      return $s;
  }

?>