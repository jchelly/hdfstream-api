import org.apache.catalina.realm.RealmBase;
import org.apache.catalina.connector.Request;
import org.apache.catalina.realm.GenericPrincipal;

import java.security.Principal;
import java.util.*;

// Authentication realm which provides a method to add users programatically,
// since the built in MemoryRealm does not. Intended for unit testing with
// embedded Tomcat.
public class UnitTestRealm extends RealmBase {

    // Users are defined as (username : password) map entries
    private final Map<String, String> userPasswords = new HashMap<>();

    // We also have a (username : array_of_role_names) map with the security
    // roles associated with each user.
    private final Map<String, List<String>> userRoles = new HashMap<>();

    public void addUser(String username, String password, String[] roles) {
        userPasswords.put(username, password);
        userRoles.put(username, Arrays.asList(roles));
    }

    @Override
    protected String getPassword(String username) {
        return userPasswords.get(username);
    }

    @Override
    protected Principal getPrincipal(String username) {
        List<String> roles = userRoles.getOrDefault(username, Collections.emptyList());
        return new GenericPrincipal(username, userPasswords.get(username), roles);
    }
}
