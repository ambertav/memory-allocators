import AllocatorModule from '../wasm/allocator_wasm.js';

let allocators;

async function init() {
  const base = new URL('../wasm/', import.meta.url).href;

  const Module = await AllocatorModule({
    locateFile: (path) => `${base}${path}`,
  });

  allocators = {
    linear: new Module.LinearAllocator(),
    'free-list': new Module.FreeListAllocator(),
    buddy: new Module.BuddyAllocator(),
  };
}

const config = {
  linear: {
    title: 'Linear Allocator',
    controls: ['allocate', 'resize_last', 'reset'],
  },
  'free-list': {
    title: 'Free List Allocator',
    controls: ['allocate', 'deallocate', 'reset'],
  },
  buddy: {
    title: 'Buddy Allocator',
    controls: ['allocate', 'deallocate', 'reset'],
  },
};

const pointers = {
  linear: new Map(),
  'free-list': new Map(),
  buddy: new Map(),
};
const SIZE = 1024;

document.addEventListener('DOMContentLoaded', async () => {
  await init();

  document.querySelectorAll('.toggle').forEach((button) => {
    button.addEventListener('click', () => {
      const type = button.dataset.type;

      const previous = document.querySelector('.selected');
      if (previous) previous.classList.remove('selected');
      button.classList.add('selected');

      renderAllocator(type);
      renderDescription(type);
      renderControls(type);
      syncPointerDropdown(type);
    });
  });

  function renderAllocator(type) {
    resetError();

    const state = JSON.parse(allocators[type].getState());

    renderBlocks(type, state);
    renderMetrics(type, state.metrics);
  }

  function renderBlocks(type, state) {
    const allocator = document.getElementById('allocator');
    const legend = document.getElementById('legend');
    const note = document.getElementById('note');

    allocator.innerHTML = '';
    allocator.className = '';

    state.blocks.forEach((block) => {
      if (block.header > 0) {
        const header = document.createElement('div');
        header.className = 'block header';
        header.style.left = `${(block.offset / state.totalBytes) * 100}%`;
        header.style.width = `${(block.header / state.totalBytes) * 100}%`;
        allocator.appendChild(header);
      }

      const element = document.createElement('div');
      element.className = `block ${block.status}`;
      element.style.left = `${((block.offset + block.header) / state.totalBytes) * 100}%`;
      element.style.width = `${(block.size / state.totalBytes) * 100}%`;

      const text = document.createElement('span');
      text.innerHTML = `${block.size} B`;
      text.style.fontSize = '0.9rem';
      element.appendChild(text);

      allocator.appendChild(element);
    });

    allocator.classList.add('view');
    legend.classList.add('view');
    note.classList.add('view');
  }

  function renderMetrics(type, metrics) {
    const items = document.querySelectorAll('.metrics-list li span');

    items.forEach((item) => {
      const key = item.id.replace('m-', '');
      item.innerHTML = `${metrics[key]}`;
    });
  }

  function renderDescription(type) {
    const info = config[type];
    document.querySelector('.title').textContent = info.title;
  }

  function renderControls(type) {
    const info = config[type];
    const controls = document.querySelector('.controls');

    let html = `<h4>Controls</h4>`;

    if (info.controls.includes('allocate')) {
      html += `
        <div class="control-row">
            <button id="allocate-button">Allocate</button>
            <input id="allocate-size" type="number" min="1" max="${SIZE / 2}" value="16" step="1" />
            <span>bytes</span>
        </div>
      `;
    }
    if (info.controls.includes('deallocate')) {
      html += `
        <div class="control-row">
            <button id="deallocate-button">Deallocate</button>
            <select id="deallocate-ptr">
                <option value="" disabled selected>- select block -</option>
            </select>
        </div>
      `;
    }
    if (info.controls.includes('resize_last')) {
      html += `
        <div class="control-row">
            <button id="resize-button">Resize Last</button>
             <input id="resize-size" type="number" min="1" max="${SIZE / 2}" value="16" step="1" />
             <span>bytes</span>
        </div>
      `;
    }
    if (info.controls.includes('reset')) {
      html += `
        <div class="control-row">
            <button id="reset-button">Reset</button>
        </div>
      `;
    }

    controls.innerHTML = html;
    attachControlHandlers(type);
  }

  function attachControlHandlers(type) {
    const allocateButton = document.getElementById('allocate-button');
    const deallocateButton = document.getElementById('deallocate-button');
    const resizeButton = document.getElementById('resize-button');
    const resetButton = document.getElementById('reset-button');

    if (allocateButton) {
      allocateButton.onclick = () => {
        resetError();

        const size = parseInt(
          document.getElementById('allocate-size').value,
          10,
        );
        const ptr = allocators[type].allocate(size);
        if (ptr === 0) {
          handleError('Allocation failed');
          return;
        }

        pointers[type].set(ptr, size);
        syncPointerDropdown(type);
        renderAllocator(type);
      };
    }

    if (deallocateButton) {
      deallocateButton.onclick = () => {
        resetError();
        const selectedOption = document.getElementById('deallocate-ptr');
        const ptr = parseInt(selectedOption.value, 10);
        if (isNaN(ptr)) {
          handleError('Could not parse pointer');
          return;
        }

        allocators[type].deallocate(ptr);
        pointers[type].delete(ptr);
        syncPointerDropdown(type);
        renderAllocator(type);
      };
    }

    if (resizeButton) {
      resizeButton.onclick = () => {
        resetError();
        const size = parseInt(document.getElementById('resize-size').value, 10);
        const previous_ptr = [...pointers[type].keys()].at(-1);
        const new_ptr = allocators[type].resizeLast(previous_ptr, size);
        if (new_ptr === 0) {
          handleError('Resize allocation failed');
          return;
        }

        pointers[type].set(new_ptr, size);
        syncPointerDropdown(type);
        renderAllocator(type);
      };
    }

    if (resetButton) {
      resetButton.onclick = () => {
        resetError();
        allocators[type].reset();
        pointers[type].clear();
        syncPointerDropdown(type);
        renderAllocator(type);
      };
    }
  }

  function syncPointerDropdown(type) {
    const pointerDropdown = document.getElementById('deallocate-ptr');
    if (!pointerDropdown) {
      return;
    }

    const prev = pointerDropdown.value;
    pointerDropdown.innerHTML = `
        <option value="" disabled selected>- select block -</option>
        ${[...pointers[type].entries()]
          .map(
            ([ptr, size]) =>
              `<option value="${ptr}">0x${ptr.toString(16)}, (${size} bytes)</option>`,
          )
          .join('')};
    `;

    if (pointers[type].has(parseInt(prev))) {
      pointerDropdown.value = prev;
    }
  }

  function handleError(message) {
    const element = document.getElementById('error');
    element.innerHTML = message;
    element.classList.add('view');
  }

  function resetError() {
    const element = document.getElementById('error');
    element.innerHTML = '';
    element.classList.remove('view');
  }
});
