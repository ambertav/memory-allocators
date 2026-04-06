/////////////////////////////////
// Navigation
/////////////////////////////////
class Navigation extends HTMLElement {
  connectedCallback() {
    const current = window.location.pathname;
    const depth = current.split('/').filter(Boolean).length;
    const isDocsPage = current.includes('/docs/');
    const root = isDocsPage ? '../' : '';

    const isActive = (href) => {
      if (href === 'index.html') {
        return (
          current === '/' ||
          current.endsWith('index.html') ||
          current.endsWith('/memory-allocators/')
        );
      }
      return current.includes(href.replace('.html', ''));
    };

    this.innerHTML = `
        <nav>
        <ul class="links">
          <li><a class="${isActive('index.html') ? 'active' : ''}" href="${root}index.html">Visualizer</a></li>
          <li><a class="${isActive('about.html') ? 'active' : ''}" href="${root}about.html">About</a></li>
          <li class="dropdown">
            <button class="dropdown-trigger ${current.includes('docs') ? 'active' : ''}">
              Documentation <span>▾</span>
            </button>
            <ul class="dropdown-menu">
              <li><a href="${root}docs/linear.html">Linear</a></li>
              <li><a href="${root}docs/freelist.html">Free List</a></li>
              <li><a href="${root}docs/buddy.html">Buddy</a></li>
            </ul>
          </li>
        </ul>
      </nav>
    `;

    const trigger = this.querySelector('.dropdown-trigger');
    const menu = this.querySelector('.dropdown-menu');

    trigger.addEventListener('click', (evt) => {
      evt.stopPropagation();
      menu.classList.toggle('open');
      trigger.classList.toggle('open');
    });

    document.addEventListener('click', () => {
      menu.classList.remove('open');
      trigger.classList.remove('open');
    });
  }
}

class Footer extends HTMLElement {
  connectedCallback() {
    const year = new Date().getFullYear();

    this.innerHTML = `
            <footer>
                <ul>
                    <li>
                        <a
                            href="https://github.com/ambertav"
                            target="_blank"
                            rel="noopener noreferrer"
                            aria-label="Github"
                            title="Github"
                        >
                            Github
                        </a>
                    </li>
                    <li>
                        <a
                            href="https://www.linkedin.com/in/ambertaveras"
                            target="_blank"
                            rel="noopener noreferrer"
                            aria-label="Linkedin"
                            title="Linkedin"
                        >
                            Linkedin
                        </a>
                    </li>
                </ul>
                <p id="copyright">built by amber taveras, ${year}</p>
            </footer>
        `;
  }
}

customElements.define('site-nav', Navigation);
customElements.define('site-footer', Footer);
